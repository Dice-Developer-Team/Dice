from tkinter import *  # 导入 Tkinter 库

json = [
    {
        "id":1,
        "name": "1",
        "function": "function1"
    },
    {
        "id":1,
        "name": "2",
        "function": "function2"
    },
    {
        "id":1,
        "name": "3",
        "function": "function3"
    }
]

gui = Tk()  # 创建窗口对象的背景色
gui.title("你家大爷")

i = 1
Label(gui, text="id").grid(row=0, column=0)
Label(gui, text="function").grid(row=0, column=1)
Label(gui, text="name").grid(row=0, column=2)
Label(gui, text="enable").grid(row=0, column=3)
for obj in json:
    obj["id"] = i

    tstr = StringVar()
    tstr.set(obj["name"])
    obj["name"] = tstr

    tint = IntVar()
    tint.set(1)
    obj["enable"] = tint

    Label(gui, text=obj.get("id")).grid(row=i, column=0)
    Label(gui, text=obj.get("function")).grid(row=i, column=1)
    Entry(gui, textvariable=tstr).grid(row=i, column=2)
    Checkbutton(gui, variable=tint).grid(row=i, column=3)

    i = i + 1

gui.mainloop()  # 进入消息循环

for obj in json:
    obj["name"]=obj["name"].get()
    obj["enable"]=obj["enable"].get()
print(json)
