#import matplotlib.pyplot as plt
import sys

out = sys.argv[1]
with open(out) as f:
	lines = f.read().splitlines()

i = 0
csv = []
while i < len(lines):
	s = lines[i].split()
	size = int(s[0])
	time = float(s[4])
	l = [size, time]
	csv.append(l) 
	i = i + 1

f = open(out.split('.')[0] + '.csv', 'w')
for c in csv:
	f.write(str(c[0]) + ', ' + str(c[1]) + '\n')

f.close()

#y = [c[1] for c in csv]
#x = [c[2] for c in csv]
#n = [c[0] for c in csv]

#fig, ax = plt.subplots()
#ax.scatter(x, y)

#for i, txt in enumerate(n):
	#ax.annotate(txt, (x[i], y[i]))
