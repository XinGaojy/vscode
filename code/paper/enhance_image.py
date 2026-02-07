import cv2
import numpy as np
from PIL import Image, ImageEnhance, ImageDraw, ImageFont
import os

# 图片路径
image_path = r'd:\APP\wsl\WSL-Ubuntu22.04\rootfs\workspace\linux\clang-quickstart\vscode\code\paper\fiber_image.png'

# 检查图片文件是否存在
if not os.path.exists(image_path):
    print(f"图片文件不存在: {image_path}")
    exit(1)

# 读取图片
img = cv2.imread(image_path)
if img is None:
    print("无法读取图片")
    exit(1)

print(f"原始图片大小: {img.shape}")

# ===== 步骤1: 调整对比度和亮度 =====
# 使用CLAHE (对比度自适应直方图均衡化)
lab = cv2.cvtColor(img, cv2.COLOR_BGR2LAB)
l, a, b = cv2.split(lab)

clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
l = clahe.apply(l)

enhanced = cv2.merge([l, a, b])
enhanced = cv2.cvtColor(enhanced, cv2.COLOR_LAB2BGR)

# ===== 步骤2: 去除噪点 =====
# 使用双边滤波器保留边缘同时去噪
denoised = cv2.bilateralFilter(enhanced, 9, 75, 75)

# ===== 步骤3: 锐化 =====
# 使用Unsharp Mask锐化
gaussian = cv2.GaussianBlur(denoised, (0, 0), 2.0)
sharpened = cv2.addWeighted(denoised, 1.5, gaussian, -0.5, 0)

# ===== 步骤4: 调整对比度 =====
# 增强对比度
kernel = np.array([[-1, -1, -1],
                   [-1,  9, -1],
                   [-1, -1, -1]]) / 1.2
sharpened = cv2.filter2D(sharpened, -1, kernel)

# ===== 步骤5: 添加科研标注 =====
# 转换为PIL格式以添加文字
pil_image = Image.fromarray(cv2.cvtColor(sharpened, cv2.COLOR_BGR2RGB))

# 获取图片尺寸
width, height = pil_image.size

# 创建绘图对象
draw = ImageDraw.Draw(pil_image)

# 添加标注 (a 和 b)
try:
    # 尝试使用更大的字体
    font_large = ImageFont.load_default()
except:
    font_large = ImageFont.load_default()

# 在左上角和右上角添加标注
draw.text((20, 20), "a", fill=(255, 255, 255), font=font_large)
draw.text((width//2 + 20, 20), "b", fill=(255, 255, 255), font=font_large)

# ===== 步骤6: 保存高分辨率图片 =====
output_path = r'd:\APP\wsl\WSL-Ubuntu22.04\rootfs\workspace\linux\clang-quickstart\vscode\code\paper\fiber_image_enhanced.png'
pil_image.save(output_path, 'PNG', quality=95)

print(f"美化后的图片已保存: {output_path}")

# 也保存为TIFF格式（300dpi）用于论文发表
tiff_path = r'd:\APP\wsl\WSL-Ubuntu22.04\rootfs\workspace\linux\clang-quickstart\vscode\code\paper\fiber_image_enhanced_300dpi.tiff'
pil_image.save(tiff_path, 'TIFF', dpi=(300, 300), quality=95)

print(f"高分辨率TIFF已保存 (300dpi): {tiff_path}")

print("\n=== 图片美化完成 ===")
print("优化内容:")
print("✓ 对比度和亮度调整 (CLAHE算法)")
print("✓ 去除噪点 (双边滤波)")
print("✓ 锐化细节 (Unsharp Mask)")
print("✓ 增强对比度")
print("✓ 添加样品标注 (a, b)")
print("✓ 导出高分辨率格式 (PNG和TIFF 300dpi)")
