import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import pandas as pd
import os
from matplotlib.colors import Normalize
import matplotlib.cm as cm

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial Unicode MS', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

def plot_memory_mountain_csapp_view(file_path):
    if not os.path.exists(file_path):
        print(f"错误: 找不到文件 {file_path}")
        return

    # 1. 读取数据
    try:
        df = pd.read_csv(file_path, header=None, names=['Size', 'Stride', 'Throughput'])
    except Exception as e:
        print(f"解析文件失败: {e}")
        return

    df = df.sort_values(by=['Size', 'Stride'])
    
    sizes = sorted(df['Size'].unique())
    strides = sorted(df['Stride'].unique())

    # 2. 准备网格数据
    # X 轴: Size (log2)
    # Y 轴: Stride
    X, Y = np.meshgrid(np.log2(sizes), strides)
    
    Z = np.zeros(X.shape)

    for i, s_val in enumerate(sizes):
        for j, st_val in enumerate(strides):
            val = df[(df['Size'] == s_val) & (df['Stride'] == st_val)]['Throughput']
            if not val.empty:
                # Z[row, col] -> Y[row] (Stride), X[col] (Size)
                Z[j, i] = val.iloc[0] 

    # 3. 绘图
    print("正在生成 XY轴互换且步长反转的存储器山图...")
    fig = plt.figure(figsize=(15, 11))
    ax = fig.add_subplot(111, projection='3d')

    # --- 颜色和光照设置 ---
    norm = Normalize(vmin=np.min(Z), vmax=np.max(Z))
    cmap = plt.get_cmap('Blues_r')
    
    surf = ax.plot_surface(X, Y, Z, 
                           cmap=cmap, 
                           norm=norm,
                           edgecolor='none', 
                           alpha=1.0,       
                           antialiased=True,
                           shade=True,      
                           lightsource=plt.matplotlib.colors.LightSource(azdeg=315, altdeg=45))

    # --- 关键修改：反转步长轴 (Y轴) ---
    
    # X 轴 (Size): 保持大容量在前 (靠近观察者)，所以反转
    ax.set_xlim(np.log2(sizes)[-1], np.log2(sizes)[0]) 
    
    # Y 轴 (Stride): 【修改点】反转步长轴
    # 原来: ax.set_ylim(strides[0], strides[-1])  (小->大)
    # 现在: ax.set_ylim(strides[-1], strides[0])  (大->小)
    # 这意味着大步长将位于轴的“起始”端（通常在左侧或后方，取决于视角），小步长在“结束”端
    ax.set_ylim(strides[-1], strides[0])

    # 视角调整：
    # 由于轴方向变了，可能需要微调 azim 以保持“最低谷在最近处”的视觉效果
    # 当前配置：X(Size)反转，Y(Stride)反转。
    # 低谷在 (大Size, 大Stride)。
    # X轴大值在左/前，Y轴大值在左/后？这取决于 view_init。
    # azim=-120 通常能很好地展示这种对角线关系。
    ax.view_init(elev=30, azim=-120) 
    
    ax.set_zlim(0, np.max(Z) * 1.05)

    # --- 标签 ---
    ax.set_xlabel('Size (Bytes)', labelpad=15, fontsize=12, fontweight='bold')
    ax.set_ylabel('Stride (x8 bytes)', labelpad=15, fontsize=12, fontweight='bold')
    ax.set_zlabel('Read Throughput (MB/s)', fontsize=12, fontweight='bold')
    ax.set_title('Memory Mountain', fontsize=14, fontweight='bold')

    # --- X 轴刻度格式化 (Size) ---
    x_ticks = np.log2(sizes)
    x_labels = []
    for val in sizes:
        if val >= 1024 * 1024:
            x_labels.append(f"{int(val / (1024 * 1024))}M")
        elif val >= 1024:
            x_labels.append(f"{int(val / 1024)}K")
        else:
            x_labels.append(f"{int(val)}")
    
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels, fontsize=10)

    ax.set_yticks(strides)
    ax.set_yticklabels(strides, fontsize=10)

    # 添加颜色条
    cbar = fig.colorbar(surf, shrink=0.5, aspect=10, label='Read Throughput (MB/s)')
    cbar.ax.tick_params(labelsize=10)
    
    ax.grid(True, linestyle='--', alpha=0.2)

    plt.tight_layout()

    output_fn = 'memory_mountain_xy_swapped_stride_reversed.png'
    plt.savefig(output_fn, dpi=300, bbox_inches='tight')
    print(f"成功生成图片: {output_fn}")
    plt.show()

if __name__ == "__main__":
    plot_memory_mountain_csapp_view('data.txt')