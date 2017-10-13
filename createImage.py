import PIL
from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw
import colorsys

font = ImageFont.truetype("/usr/share/fonts/truetype/DejaVuSans.ttf",275)

#RGB represent a 4 byte int
img_std=Image.new("RGBA", (1920,1080),(0, 0, 0, 0))

#This is the actual color
img=Image.new("RGBA", (1920,1080),(255,255,255,255))

draw = ImageDraw.Draw(img)
textcolor = list(colorsys.hsv_to_rgb(200/360.0, 1, 1))
textcolor.append(1)
textcolor = tuple([int(color*255) for color in textcolor])
draw.text((1000, 100),"aMuDi",textcolor,font=font)

draw_std = ImageDraw.Draw(img_std)
draw_std.text((1000, 100),"aMuDi",(255, 255, 255, 255),font=font)

img = img.convert('RGBA')
img.save("test.png")
img_std.save("test_std.png")
