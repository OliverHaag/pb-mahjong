#!/usr/bin/env python3

from wand.color import Color
from wand.image import Image
import sys
from xml.dom.minidom import parse

if len (sys.argv) != 3:
	print(f'Usage: {sys.argv[0]} <inputfile> <outputfile>')
	sys.exit(1)
    
svg = parse(sys.argv[1])

g = svg.getElementsByTagName('g')[0]
for path in g.getElementsByTagName('path'):
	if not 'style' in path.attributes:
		g.removeChild(path)
		break

with Image(blob=svg.toxml().encode('utf-8')) as image:
	image.format = 'bmp'
	image.background_color = Color('#eeeeee')
	image.alpha_channel = 'remove'
	with image.fx('sqrt((r^2+g^2+b^2)/3)') as grayscale:
		grayscale.type = 'grayscale'
		grayscale.save(filename=sys.argv[2])
