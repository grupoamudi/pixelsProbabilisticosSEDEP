from PIL import Image
import colorsys
import struct


filebase = 'test'
img = Image.open(filebase + ".png") 
img_std = Image.open(filebase + "_std.png")

pix = img.load()
pix_std = img_std.load()
if img.size != img_std.size:
    raise Exception("Images should have the same size")
print(img.size) #Get the width and hight of the image for iterating over
result = open(filebase+'.res', 'wb')
for y in range(img.size[1]):
    print(y)
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
