import pydicom, sys
from colorama import Fore, Style, init

# colorama
init()

d = pydicom.read_file(sys.argv[1])
if len(sys.argv)>2:
  frame = int(sys.argv[2])-1
  print("Dumping frame "+str(frame))
else:
  frame = None
print(d.Rows)
print(d.Columns)
print(d.NumberOfFrames)

totalPixels = int(d.Rows*d.Columns*d.NumberOfFrames/8)
if totalPixels%8:
  totalPixels = totalPixels + 1
totalPixels = totalPixels + (totalPixels % 2)
print("Total pixels expected: %i" % totalPixels)

print("Total pixels actual: %i" % len(d.PixelData))

if not frame is None:
  frames = [frame]
else:
  frames = range(d.NumberOfFrames)



import numpy as np
unpacked = np.unpackbits(np.frombuffer(d.PixelData,dtype=np.uint8))

print("With numpy unpackbits:")

for f in frames:
  print("Frame %i" % f)
  for i in range(d.Rows):
    for j in range(d.Columns):
      pixelNumber = f*d.Rows*d.Columns+i*d.Columns+j
      if int(pixelNumber/8)%2:
        sys.stdout.write(Fore.RED)
      else:
        sys.stdout.write(Fore.WHITE)
      if unpacked[pixelNumber]:
        sys.stdout.write("X")
      else:
        sys.stdout.write(".")
    print("")

print("\nWith manual unpacking:")
for f in frames:
  print("Frame %i" % f)
  for i in range(d.Rows):
    for j in range(d.Columns):
      pixelNumber = f*d.Rows*d.Columns+i*d.Columns+j
      byteNumber = int(pixelNumber/8)
      bitPosition = pixelNumber % 8
      if byteNumber%2:
        sys.stdout.write(Fore.RED)
      else:
        sys.stdout.write(Fore.WHITE)
      if (d.PixelData[byteNumber] >> bitPosition) & 1:
        sys.stdout.write("X")
      else:
         sys.stdout.write(".")
    print("")
print(Style.RESET_ALL)
