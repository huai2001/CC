# -*- coding:utf-8 -*-
import os
import sys
# 下载安装
# pip install chardet
import chardet
import codecs

CTypeList = {".c", ".cc", ".cpp", ".h",".txt"}

def GetFileExtension(file):
    (filepath, filename) = os.path.split(file)
    (shortname, extension) = os.path.splitext(filename)
    return extension.lower()

def readFileContext(file):
    
  
    if (len(fileContext) <= 0):
        return 0

    return fileContext

def transcoding(filename, coding):
    try:
        fileContext = codecs.open(filename, 'r').read()
        encoding = chardet.detect(fileContext)["encoding"]
        if (encoding == None):
            print("Skipped empty file" + filename)
            return

        if coding != 'utf-8' and coding != 'gb2312':
            print("file encoding:" + filename + " " + encoding)
            return;
        if (encoding.lower() != coding):
            fileContext = fileContext.decode(encoding);
            codecs.open(filename, 'w', coding).write(fileContext)
            print ("convert:" + filename + " sucess "+ encoding + " "+coding)
    except IOError as err:
        print ("I/O error: {0}".format(err))

def encodingFile(r, coding):
    list = os.listdir(r) #列出文件夹下所有的目录与文件
    for i in range(0, len(list)):
        fullname = os.path.join(r, list[i])
        if os.path.isfile(fullname):
            fileExtension = GetFileExtension(fullname)
            if fileExtension in CTypeList:
                transcoding(fullname, coding)
        else :
            encodingFile(fullname, coding)

if __name__ == "__main__":
    currPath = sys.path[0];
    if (len(sys.argv) > 1):
        if (sys.argv[1] == 'UTF-8' or sys.argv[1] == 'GB2312'):
            encodingFile(currPath, sys.argv[1].lower())
    else:
        encodingFile(currPath,'')