from socket import *
from struct import *

#import alsaaudio, time, audioop

crc_tab = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
]


def check_crc(packet):

    crc = 0

    mask = 255 # mask 8 bit 

    for x in range(0, len(packet)-2):

        dat = (crc >> 8) & mask # get last 8 bit

        crc <<= 8
        
        crc ^= crc_tab[dat ^ packet[x]]

    first_byte = ((crc >> 8) & mask)

    second_byte = crc & mask # get last 8 bit

    if ((packet[len(packet)-2] == first_byte) & (packet[len(packet)-1] == second_byte)):
        return True

    return False


def pack_crc(packet):

    crc = 0

    mask = 255 # mask 8 bit 

    for x in range(0, len(packet)-2):
        print '1', crc
        dat = (crc >> 8) & mask # get last 8 bit

        crc <<= 8
        print '2', crc, ' - ', dat, ' - ', packet[x], ' - ', (dat ^ packet[x])
        crc ^= crc_tab[dat ^ packet[x]]
        print '3', crc

    packet[len(packet)-2] = ((crc >> 8) & mask) # get last 8 bit

    packet[len(packet)-1] = crc & mask


hdl_proto_packet = []

hdl_proto_packet.append(0xC0)
hdl_proto_packet.append(0xA8)
hdl_proto_packet.append(0x0A)
hdl_proto_packet.append(0x64)
hdl_proto_packet.append(0x48) # H
hdl_proto_packet.append(0x44) # D
hdl_proto_packet.append(0x4C) # L
hdl_proto_packet.append(0x4D) # M
hdl_proto_packet.append(0x49) # I
hdl_proto_packet.append(0x52) # R
hdl_proto_packet.append(0x41) # A
hdl_proto_packet.append(0x43) # C
hdl_proto_packet.append(0x4C) # L
hdl_proto_packet.append(0x45) # E
hdl_proto_packet.append(0xAA) # leading code
hdl_proto_packet.append(0xAA) # leading code

data_packet = []

data_packet.append(0x0D) # data length
data_packet.append(0x01) # subnet id
data_packet.append(0xFE) # device id
data_packet.append(0xFF) # original device type: higher then 8
data_packet.append(0xFE) # original device type: lower then 8
data_packet.append(0x00) # operate code: higher then 8
data_packet.append(0x31) # Openerate code: lower then 8
data_packet.append(0x01) # subnet ID of targeted device
data_packet.append(0x5F) # device ID of targeted device
data_packet.append(0x03) # additional, channel No
data_packet.append(0x28) # additional, intensity
data_packet.append(0x00) # CRC, higher then 8
data_packet.append(0x00) # CRC, lower then 8

# generate check_sum
pack_crc(data_packet)

udp_packet = hdl_proto_packet + data_packet

hexstring = ''

for x in range(0, len(udp_packet)):
   hexstring += pack('B', udp_packet[x])

#print pack('B', hdl_proto_packet)

sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
sock.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)

sock.sendto(hexstring, ('192.168.10.255', 6000))






# Open the device in nonblocking capture mode. The last argument could
# just as well have been zero for blocking mode. Then we could have
# left out the sleep call in the bottom of the loop
#inp = alsaaudio.PCM(alsaaudio.PCM_CAPTURE,alsaaudio.PCM_NONBLOCK)

# Set attributes: Mono, 8000 Hz, 16 bit little endian samples
#inp.setchannels(1)
#inp.setrate(8000)
#inp.setformat(alsaaudio.PCM_FORMAT_S16_LE)

# The period size controls the internal number of frames per period.
# The significance of this parameter is documented in the ALSA api.
# For our purposes, it is suficcient to know that reads from the device
# will return this many frames. Each frame being 2 bytes long.
# This means that the reads below will return either 320 bytes of data
# or 0 bytes of data. The latter is possible because we are in nonblocking
# mode.
#inp.setperiodsize(160)

#while True:
#    # Read data from device
#    l,data = inp.read()
#    if l:
#        # Return the maximum of the absolute value of all samples in a fragment.
#        max = audioop.max(data, 2)
#        #print max
#        if (max > 28000):
#                print max
#                sock.sendto(hexstring, ('192.168.10.255', 6000))
#    time.sleep(.001)



