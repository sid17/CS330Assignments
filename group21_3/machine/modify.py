import sys 
filenamepart = "machine" 
macro = '#define NumPhysPages ' 
def main(): 
  BOUND = int(sys.argv[1]) 
  filename = filenamepart + ".h" 
  f1 = open(filename, 'r') 
  code = "" 
  for line in f1: 
    #print "read>" + line, 
    if (line.find(macro) == -1):
      #print "writ>" + line, 
      code += line 
    else: 
      macrofull = macro + str(BOUND) + "\n" 
      #print "writ>" + macrofull, 
      code += macrofull 
  f1.close() 
  f2 = open(filename, 'w') 
  #print code 
  f2.write(code) 
  f2.close() 
  pass

if __name__ == "__main__": main()
