import os


def listCppInDir(dir=""):
    if dir == "":
        dir = os.getcwd()
    # print(dir)
    files = []
    for file in os.listdir(dir):
        dirfile = 目录加后斜杠(dir) + file
        if os.path.isdir(dirfile):  # 扫描子目录
            if file == "CQSDK":  # 不扫描CQSDK目录
                continue
            file1 = listCppInDir(dirfile)
            files.extend(file1)
        elif file.endswith(".cpp"):
            # print("    cpp : "+file)
            files.append(dirfile)
        # else:
            # print("    文件: "+file)
    return files


def 目录加后斜杠(dir):
    if not dir.endswith('\\'):
        return dir + '\\'
    return dir


if __name__ == "__main__":
    print(listCppInDir())
