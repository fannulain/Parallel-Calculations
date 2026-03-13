import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('test_results.csv')
sizes = sorted(df['Vector Size'].unique())

plt.style.use('bmh')

fig, axes = plt.subplots(nrows=2, ncols=4, figsize=(20, 10))
axes = axes.flatten()

strategies = ['Mutex Unoptimized', 'CAS Unoptimized', 'Mutex Optimized', 'CAS Optimized']
markers = ['^', 'v', 's', 'o']
colors = ['red', 'orange', 'green', 'blue']

for i, size in enumerate(sizes):
    ax = axes[i]
    df_size = df[df['Vector Size'] == size]

    default_time = df_size[df_size['Strategy'] == 'Default (1 Thread)']['Total Time (s)'].values[0]
    ax.axhline(y=default_time, color='black', linestyle='--', label='Default (1 Thread)')

    for strat, marker, color in zip(strategies, markers, colors):
        subset = df_size[df_size['Strategy'] == strat]
        ax.plot(subset['Threads'], subset['Total Time (s)'], 
                label=strat, marker=marker, color=color, linewidth=2)
        
    ax.set_title(f'Vector size: {size:,}')
    ax.set_xlabel('Threads number')
    ax.set_ylabel('Total time (с)')
    ax.set_yscale('log')
    ax.set_xscale('log', base=2)

    ax.set_xticks([5, 10, 16, 32, 64, 128, 256])
    ax.set_xticklabels(['5', '10', '16', '32', '64', '128', '256'])
    ax.tick_params(axis='x', rotation=0)
    ax.grid(True, which="both", ls="--", alpha=0.5)

fig.delaxes(axes[7])

handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, loc='lower right', bbox_to_anchor=(0.95, 0.2), fontsize=14)

plt.tight_layout()
plt.subplots_adjust(bottom=0.1)
plt.savefig('1_threads_time_combined.png', dpi=300)
plt.close()

size_labels = [f"{int(s/1000000)}m" if s >= 1000000 else f"{int(s/1000)}k" for s in sizes]

df_16 = df[df['Threads'] == 16]
df_default = df[df['Strategy'] == 'Default (1 Thread)']

plt.figure(figsize=(10, 6))

plt.plot(df_default['Vector Size'], df_default['Total Time (s)'], 
         label='Default (1 Thread)', marker='o', color='black', linewidth=2)

opt_strategies = ['Mutex Optimized', 'CAS Optimized']
opt_markers = ['s', 'o']
opt_colors = ['green', 'blue']

for strat, marker, color in zip(opt_strategies, opt_markers, opt_colors):
    subset = df_16[df_16['Strategy'] == strat]
    plt.plot(subset['Vector Size'], subset['Total Time (s)'], 
             label=strat, marker=marker, color=color, linewidth=2)

plt.title("Relationship between time and vector size (16 threads) - Optimized")
plt.xlabel("Vector size")
plt.ylabel("Total time (с)")
plt.grid(True, which="both", ls="--")
plt.xscale('log')
plt.xticks(sizes, size_labels, rotation=45)
plt.legend()
plt.tight_layout()
plt.savefig('2_size_time_optimized.png', dpi=300)
plt.close()

plt.figure(figsize=(10, 6))

plt.plot(df_default['Vector Size'], df_default['Total Time (s)'], 
         label='Default (1 Thread)', marker='o', color='black', linewidth=2)

unopt_strategies = ['Mutex Unoptimized', 'CAS Unoptimized']
unopt_markers = ['^', 'v']
unopt_colors = ['red', 'orange']

for strat, marker, color in zip(unopt_strategies, unopt_markers, unopt_colors):
    subset = df_16[df_16['Strategy'] == strat]
    plt.plot(subset['Vector Size'], subset['Total Time (s)'], 
             label=strat, marker=marker, color=color, linewidth=2)

plt.title("Relationship between time and vector size (16 threads) - UnOptimized")
plt.xlabel("Vector size")
plt.ylabel("Total time (с)")
plt.grid(True, which="both", ls="--")
plt.xscale('log')
plt.xticks(sizes, size_labels, rotation=45)
plt.legend()
plt.tight_layout()
plt.savefig('3_size_time_unoptimized.png', dpi=300)
plt.close()

print("Success!")