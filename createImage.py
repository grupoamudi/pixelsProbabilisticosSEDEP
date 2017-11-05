import PIL
from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw
import colorsys
import struct

font = ImageFont.truetype("/usr/share/fonts/truetype/DejaVuSans.ttf",200)

letter_size = 66*2
words = ['aMuDi', 'SEDEP']

for word in words:
    size = (int(letter_size*len(word)), 106*2)
    #RGB represent a 4 byte int
    img_std=Image.new("RGBA", size,(0, 0, 0, 0))
    
    #This is the actual color
    img=Image.new("RGBA", size,(255,255,255,255))
    
    draw = ImageDraw.Draw(img)
    textcolor = list(colorsys.hsv_to_rgb(200/360.0, 1, 1))
    textcolor.append(1)
    textcolor = tuple([int(color*255) for color in textcolor])
    draw.text((0, 0),word,textcolor,font=font)
    
    draw_std = ImageDraw.Draw(img_std)
    draw_std.text((0, 0),word,(255, 255, 255, 255),font=font)
    
    img = img.convert('RGBA')
    img.save(word + ".png")
    img_std.save(word + "_std.png")

    img = Image.open(word + ".png") 
    img_std = Image.open(word + "_std.png")
    
    pix = img.load()
    pix_std = img_std.load()
    if img.size != img_std.size:
        raise Exception("Images should have the same size")
    result = open(word + '.res', 'wb')
    result.write(struct.pack('<H',img.size[0]))
    result.write(struct.pack('<H',img.size[1]))
    for y in range(img.size[1]):
        for x in range(img.size[0]):
            # For std image we have to convert RGBA to a 4 byte int
            rgba = pix_std[x, y]
            bvalues = []
            for val in rgba:
                bvalues.append(struct.pack('>B', val))
            for bvalue in reversed(bvalues):
                result.write(bvalue)
    
            # For normal image we have to convert RGB to HSV, but if A is deferent than 
            rgba = pix[x, y]
            hue = colorsys.rgb_to_hsv(rgba[0]/255.0, rgba[1]/255.0, rgba[2]/255.0)[0]*360
            #Need to convert to 2 byte
            hue = struct.pack('>H', int(hue))
            result.write(hue)
    result.close()

#-------------------------
# load some other patterns
patterns = ['amudimon', 'fullamudi']
for pattern in patterns:
    img = Image.open(pattern + ".png") 
    
    pix = img.load()
    result = open(pattern + '.res', 'wb')
    result.write(struct.pack('<H',img.size[0]))
    result.write(struct.pack('<H',img.size[1]))
    for y in range(img.size[1]):
        for x in range(img.size[0]):
            # For std image we have to convert RGBA to a 4 byte int
            rgba = pix[x, y]
            if sum(rgba) < 300:
                rgba = (255,255,255,255)
            else:
                rgba = (0,0,0,0)
            #if sum(rgba) > 0:
            #    bvalue = 1000
            #else:
            #    bvalue = 5
            bvalues = []
            for val in rgba:
                bvalues.append(struct.pack('>B', val))
            for bvalue in reversed(bvalues):
                result.write(bvalue)
    
            # For normal image we have to convert RGB to HSV, but if A is deferent than 
            rgba = pix[x, y]
            hue = colorsys.rgb_to_hsv(rgba[0]/255.0, rgba[1]/255.0, rgba[2]/255.0)[0]*360
            #Need to convert to 2 byte
            hue = struct.pack('>H', int(hue))
            result.write(hue)
    result.close()

