#include <cstdio>
#include <cstdlib>
#include <vector>

#include "hsa/hsa.h"
#include "hsa/hsa_ext_amd.h"

static void handle_error_impl(const char *file, int32_t line,
                              hsa_status_t code) {
  if (code == HSA_STATUS_SUCCESS || code == HSA_STATUS_INFO_BREAK)
    return;

  const char *desc;
  if (hsa_status_string(code, &desc) != HSA_STATUS_SUCCESS)
    desc = "Unknown error";
  fprintf(stderr, "%s:%d:0: Error: %s\n", file, line, desc);
  exit(EXIT_FAILURE);
}
#define handle_error(X) handle_error_impl(__FILE__, __LINE__, X)

template <typename elem_ty, typename func_ty, typename callback_ty>
hsa_status_t iterate(func_ty func, callback_ty cb) {
  auto l = [](elem_ty elem, void *data) -> hsa_status_t {
    callback_ty *unwrapped = static_cast<callback_ty *>(data);
    return (*unwrapped)(elem);
  };
  return func(l, static_cast<void *>(&cb));
}

template <typename callback_ty>
hsa_status_t iterate_agents(callback_ty callback) {
  return iterate<hsa_agent_t>(hsa_iterate_agents, callback);
}

template <hsa_device_type_t flag>
hsa_status_t get_agent(hsa_agent_t *output_agent) {
  // Find the first agent with a matching device type.
  auto cb = [&](hsa_agent_t hsa_agent) -> hsa_status_t {
    hsa_device_type_t type;
    hsa_status_t status =
        hsa_agent_get_info(hsa_agent, HSA_AGENT_INFO_DEVICE, &type);
    if (status != HSA_STATUS_SUCCESS)
      return status;

    if (type == flag) {
      // Ensure that a GPU agent supports kernel dispatch packets.
      if (type == HSA_DEVICE_TYPE_GPU) {
        hsa_agent_feature_t features;
        status =
            hsa_agent_get_info(hsa_agent, HSA_AGENT_INFO_FEATURE, &features);
        if (status != HSA_STATUS_SUCCESS)
          return status;
        if (features & HSA_AGENT_FEATURE_KERNEL_DISPATCH)
          *output_agent = hsa_agent;
      } else {
        *output_agent = hsa_agent;
      }
      return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
  };

  return iterate_agents(cb);
}

std::vector<char> read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file)
    return {};

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::vector<char> buffer(size);

  fread(buffer.data(), 1, size, file);
  fclose(file);
  return buffer;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "USAGE: ./hsa_test <filename>");
    return 1;
  }

  std::vector<char> image = read_file(argv[1]);
  // Initialize the HSA runtime used to communicate with the device.
  if (hsa_status_t err = hsa_init())
    handle_error(err);

  // Obtain a single agent for the device and host to use the HSA memory model.
  hsa_agent_t dev_agent;
  if (hsa_status_t err = get_agent<HSA_DEVICE_TYPE_GPU>(&dev_agent))
    handle_error(err);

  // Load the code object's ISA information and executable data segments.
  hsa_code_object_t object;
  if (hsa_status_t err =
          hsa_code_object_deserialize(image.data(), image.size(), "", &object))
    handle_error(err);

  hsa_executable_t executable;
  if (hsa_status_t err = hsa_executable_create_alt(
          HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO, "",
          &executable))
    handle_error(err);

  if (hsa_status_t err =
          hsa_executable_load_code_object(executable, dev_agent, object, ""))
    handle_error(err);

  // No modifications to the executable are allowed  after this point.
  if (hsa_status_t err = hsa_executable_freeze(executable, ""))
    handle_error(err);

  // Check the validity of the loaded executable. If the agents ISA features do
  // not match the executable's code object it will fail here.
  uint32_t result;
  if (hsa_status_t err = hsa_executable_validate(executable, &result))
    handle_error(err);
  if (result)
    handle_error(HSA_STATUS_ERROR);

  if (hsa_status_t err = hsa_shut_down())
    handle_error(err);

  return 0;
}
