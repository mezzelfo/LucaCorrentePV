with open("data") as f:
	for line in f:
		if "white" in line:
			points = line.split(";")
			#print("new",points[1],points[-1])
			for p in points[1:-1]:
				[x,y] = p.split(",")
				print(y,end=",")

