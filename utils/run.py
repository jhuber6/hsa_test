import os
import sys
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
import multiprocessing

def run_command(index, command):
    try:
        # Print before launching the command
        print(f"Launching on CPU {index}")
        
        # Run the command and wait for it to complete
        result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        # Print after the command is completed
        print(f"Completed on CPU {index}")
        
        return result.stdout.decode('utf-8')
    except subprocess.CalledProcessError as e:
        print(f"Error on CPU {index}: {e.stderr.decode('utf-8')}")
        exit(1)

def main():
    command = sys.argv[1:]

    # Get the number of available CPU threads
    num_threads = multiprocessing.cpu_count()
    
    while True:
        # Launch `amdhsa-loader a.out` on all available threads
        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = [executor.submit(run_command, i, command) for i in range(num_threads)]
            
            # Wait for all jobs to complete
            for future in as_completed(futures):
                result = future.result()
                if result:
                    print(result)
        print(f"Done executing all {num_threads} jobs")

if __name__ == "__main__":
    main()
