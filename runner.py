import subprocess
import os
import matplotlib.pyplot as plt
import numpy as np

def compile_program(source_dir, filename, build_dir, output_name, flags=""):
    source_path = os.path.join(source_dir, filename)
    output_path = os.path.join(build_dir, output_name)
    compile_command = f"g++ -o {output_path} {source_path} `pkg-config --cflags --libs opencv4` {flags}"
    subprocess.run(compile_command, shell=True, check=True)

def run_program(executable, video_file, show=False, num_threads=None):
    run_command = f"./{executable} {video_file}" + (" show" if show else "")
    if num_threads is not None:
        run_command += f" {num_threads}"
    result = subprocess.run(run_command, shell=True, check=True, capture_output=True, text=True)
    return result.stdout

def extract_execution_time(output):
    for line in output.splitlines():
        if "execution time" in line:
            return float(line.split()[-2])  # Assuming the time is the second last word
    return None

def plot_individual_results(results, video_names, plot_dir):
    labels = ['Sequential', 'Pthreads', 'OpenMP']
    
    for i, video_name in enumerate(video_names):
        fig, ax1 = plt.subplots()
        video_results = results[video_name]

        execution_times = video_results['times']
        speedups = video_results['speedups']

        color = 'tab:blue'
        ax1.set_xlabel('Version')
        ax1.set_ylabel('Execution Time (seconds)', color=color)
        ax1.bar(labels, execution_times, color=color, alpha=0.6, label='Execution Time')
        ax1.tick_params(axis='y', labelcolor=color)

        ax2 = ax1.twinx()
        color = 'tab:red'
        ax2.set_ylabel('Speedup', color=color)
        ax2.plot(labels, speedups, color=color, marker='o', label='Speedup')
        ax2.tick_params(axis='y', labelcolor=color)

        fig.tight_layout()
        plt.title(f'Execution Time and Speedup Comparison - {video_name}')
        plt.savefig(os.path.join(plot_dir, f'execution_time_and_speedup_comparison_{video_name}.png'))
        plt.close(fig)

def plot_combined_results(results, video_names, plot_dir):
    labels = ['Sequential', 'Pthreads', 'OpenMP']
    x = np.arange(len(labels))
    width = 0.2

    fig, ax1 = plt.subplots()

    for i, video_name in enumerate(video_names):
        video_results = results[video_name]
        execution_times = video_results['times']
        ax1.bar(x + i * width, execution_times, width, label=f'{video_name}')

    ax1.set_xlabel('Version')
    ax1.set_ylabel('Execution Time (seconds)')
    ax1.set_xticks(x + width * (len(video_names) - 1) / 2)
    ax1.set_xticklabels(labels)
    ax1.legend()

    fig.tight_layout()
    plt.title('Execution Time Comparison Across Videos')
    plt.savefig(os.path.join(plot_dir, 'execution_time_comparison_across_videos.png'))
    plt.close(fig)

def plot_average_results(results, video_names, plot_dir):
    labels = ['Sequential', 'Pthreads', 'OpenMP']

    avg_times = np.mean([results[video]['times'] for video in video_names], axis=0)
    avg_speedups = np.mean([results[video]['speedups'] for video in video_names], axis=0)

    fig, ax1 = plt.subplots()

    color = 'tab:blue'
    ax1.set_xlabel('Version')
    ax1.set_ylabel('Execution Time (seconds)', color=color)
    ax1.bar(labels, avg_times, color=color, alpha=0.6, label='Execution Time')
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()
    color = 'tab:red'
    ax2.set_ylabel('Speedup', color=color)
    ax2.plot(labels, avg_speedups, color=color, marker='o', label='Speedup')
    ax2.tick_params(axis='y', labelcolor=color)

    fig.tight_layout()
    plt.title('Average Execution Time and Speedup Comparison')
    plt.savefig(os.path.join(plot_dir, 'average_execution_time_and_speedup_comparison.png'))
    plt.close(fig)

def plot_speedup_by_threads(speedup_data, plot_dir):
    fig, ax = plt.subplots()

    for version, speedups in speedup_data.items():
        ax.plot(speedups['threads'], speedups['values'], marker='o', label=version)

    ax.set_xlabel('Number of Threads')
    ax.set_ylabel('Average Speedup')
    ax.set_title('Average Speedup by Number of Threads')
    ax.legend()
    plt.savefig(os.path.join(plot_dir, 'speedup_by_threads.png'))
    plt.close(fig)

def main():
    video_files = ["video1.mp4", "video2.mp4", "video3.mp4", "video4.mp4"]
    source_dir = "cpp_source"
    build_dir = "out"
    plot_dir = "plots"
    thread_counts = [6, 8, 10, 12]  # Different numbers of threads to test

    # Create build and plot directories if they don't exist
    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(plot_dir, exist_ok=True)

    # Compile the programs
    compile_program(source_dir, "video_filter_sequential.cpp", build_dir, "video_filter_sequential")
    compile_program(source_dir, "video_filter_pthreads.cpp", build_dir, "video_filter_pthreads", "-lpthread")
    compile_program(source_dir, "video_filter_openmp.cpp", build_dir, "video_filter_openmp", "-fopenmp")

    speedup_data = {
        'Pthreads': {'threads': [], 'values': []},
        'OpenMP': {'threads': [], 'values': []}
    }

    for num_threads in thread_counts:
        results = {}

        for video_file in video_files:
            # Run the programs and get execution times
            sequential_output = run_program(os.path.join(build_dir, "video_filter_sequential"), os.path.join('videos', video_file), show=False)
            pthreads_output = run_program(os.path.join(build_dir, "video_filter_pthreads"), os.path.join('videos', video_file), show=False, num_threads=num_threads)
            openmp_output = run_program(os.path.join(build_dir, "video_filter_openmp"), os.path.join('videos', video_file), show=False, num_threads=num_threads)

            sequential_time = extract_execution_time(sequential_output)
            pthreads_time = extract_execution_time(pthreads_output)
            openmp_time = extract_execution_time(openmp_output)

            # Calculate speedup
            speedup_pthreads = sequential_time / pthreads_time if pthreads_time else None
            speedup_openmp = sequential_time / openmp_time if openmp_time else None

            print(f"Video: {video_file} | Threads: {num_threads}")
            print(f"Sequential Time: {sequential_time} seconds")
            print(f"Pthreads Time: {pthreads_time} seconds")
            print(f"OpenMP Time: {openmp_time} seconds")
            print(f"Speedup Pthreads: {speedup_pthreads}")
            print(f"Speedup OpenMP: {speedup_openmp}")

            results[video_file] = {
                'times': [sequential_time, pthreads_time, openmp_time],
                'speedups': [1, speedup_pthreads, speedup_openmp]
            }

        # Calculate average speedup for current thread count
        avg_speedup_pthreads = np.mean([results[video]['speedups'][1] for video in video_files])
        avg_speedup_openmp = np.mean([results[video]['speedups'][2] for video in video_files])

        speedup_data['Pthreads']['threads'].append(num_threads)
        speedup_data['Pthreads']['values'].append(avg_speedup_pthreads)
        speedup_data['OpenMP']['threads'].append(num_threads)
        speedup_data['OpenMP']['values'].append(avg_speedup_openmp)

        # Plot individual results for current thread count
        plot_individual_results(results, video_files, plot_dir)

    # Plot combined results
    plot_combined_results(results, video_files, plot_dir)

    # Plot average results
    plot_average_results(results, video_files, plot_dir)

    # Plot speedup by threads
    plot_speedup_by_threads(speedup_data, plot_dir)

if __name__ == "__main__":
    main()
