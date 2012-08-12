from visual import *
import serial
import string
import math
import numpy

ser = serial.Serial('COM6', 4800, timeout=1)

scene.range=(2,2,1)
scene.forward = (1.0,0.0,0.3)
scene.up=(0,0,-1)
scene.lights = []
lamp=distant_light(direction=(0.5, 0.5, 1), color=color.gray(0.7)) 
lamp=distant_light(direction=(-0.5, -0.5, -1), color=color.gray(1))

arrow(color=color.green,axis=(2,0,0), shaftwidth=0.02, fixedwidth=1)
arrow(color=color.yellow,axis=(0,2,0), shaftwidth=0.02 , fixedwidth=1)
arrow(color=color.blue,axis=(0,0,1), shaftwidth=0.02, fixedwidth=1)

wing = box(size=(.5,.05,3), color=color.yellow)
fuse = box(size=(2,.2,.2), color=color.green)

mylabel = label(pos=(-.5,0,1),height=10,box=0)
hexadecimal = "0123456789ABCDEF"
rm = numpy.zeros(shape = (3,3))

log = open("log.txt", 'r')

while 1:
    line = log.readline()
    if len(line) == 0:
        break
    elif line[0] == '*':    # DCM
        Words = string.split(line)
        if len(Words) == 10:
            for y in range (3):
                for x in range (3):
                    k = x + (y * 3) + 1
                    temp = int(Words[k],16)/32767.
                    if temp > 1.:
                        temp = temp - 2.
                    rm[y][x] = temp

            axis = (rm[0][0], rm[1][0], rm[2][0])
            up   = (rm[0][2], rm[1][2], rm[2][2])
                
            wing.axis=axis
            wing.up=up
            wing.size=(.5,.05,3)
            fuse.axis=axis
            fuse.up=up
            fuse.size=(2,.2,.2)

            message =  str(rm[0][0])+ " " + str(rm[0][1])+ " " + str(rm[0][2])+ "\n"
            message += str(rm[1][0])+ " " + str(rm[1][1])+ " " + str(rm[1][2])+ "\n"
            message += str(rm[2][0])+ " " + str(rm[2][1])+ " " + str(rm[2][2])+ "\n"
            mylabel.text = message
            rate(20.0)

log.close
