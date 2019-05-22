
from PIL import Image

T = '0043ea78.bin'
STEP = 4

PIC = 'pwr1_hod_01_0.bmp'
W = 256
H = 192

def transform(input, tdata, output, start = 0):
	for y in range(0, H):
		for x in range(0, W):
			offset = x + ord(tdata[start % len(tdata)])
			start += 1
			output.putpixel((x, y), input.getpixel((offset % W, y)))

input  = Image.open(PIC).convert('RGB')
tdata  = file(T).read()
output = Image.new('RGB', (W, H))

assert input.size[0] == W and input.size[1] == H

step = 0
while step * STEP < 256:
	transform(input, tdata, output, step * STEP)
	output.save('transform_%02d.png' % step)
	# output.show()
	step += 1
