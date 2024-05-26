import sys
import re
import os
import os.path
import ntpath
sys.dont_write_bytecode = True

class WordInfo:
	ocurrences = 0
	inFiles = 0

wordsStat = {}

def scanFile(full_name):
  wordsInFile = set()
  f1 = open(full_name, "r")
  for line in f1.readlines():
    words = re.findall("[A-Za-z']+", line)
    for word in words:
      word = word.lower()   
      if word in wordsStat:
        wordsStat[word].ocurrences = wordsStat[word].ocurrences + 1
        if not word in wordsInFile:
          wordsStat[word].inFiles = wordsStat[word].inFiles + 1
      else:
        wordsStat[word] = WordInfo()
        wordsStat[word].ocurrences = 1
        wordsStat[word].inFiles = 1
      wordsInFile.add(word)
  f1.close()


if __name__ == '__main__':
  for path, dirs, files in os.walk("Subtitles"):
    for file_name in files:
      full_name = os.path.join(path, file_name)
      print(full_name)
      scanFile(full_name)
  listToSort = []
  totalWords = 0
  skippedWords = 0
  for key in wordsStat:
    totalWords = totalWords + 1
    #if wordsStat[key].ocurrences > 1:
    listToSort.append( (key, wordsStat[key]) )
    #else:
    #  skippedWords = skippedWords + 1
  listToSort.sort( key=lambda b: -b[1].ocurrences*b[1].inFiles )
  fileToWrite = open("ResultWords.txt", "w")
  for word in listToSort:
     fileToWrite.write(word[0])
     str = '  {},{}\n'.format(word[1].ocurrences, word[1].inFiles)
     fileToWrite.write(str)
  fileToWrite.close()
  print(totalWords)
  print(skippedWords)
