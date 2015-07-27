import sys
import urllib2

f = open("record_speech2", "r")
string = f.read()
f.close()

# see http://sebastian.germes.in/blog/2011/09/googles-speech-api/ for a good description of the url
url = 'http://www.google.com/speech-api/v1/recognize?xjerr=1&pfilter=1&client=chromium&lang=ru-RU&maxresults=4'
header = {'Content-Type' : 'audio/x-speex-with-header-byte; rate=16000'}
req = urllib2.Request(url, string, header)
data = urllib2.urlopen(req)
print 'Google says:'
print data.read()