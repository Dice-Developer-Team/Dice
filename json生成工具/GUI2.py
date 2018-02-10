from tkinter import *  # 导入 Tkinter 库

root = Tk()

标题 = Frame()
标题.grid(row=0, column=0)
事件 = Button(标题, text='事件', height=1, width=5)
事件.grid(row=0, column=0)
菜单 = Button(标题, text='菜单', height=1, width=5)
菜单.grid(row=0, column=1)
悬浮框 = Button(标题, text='悬浮框', height=1, width=5)
悬浮框.grid(row=0, column=2)
权限 = Button(标题, text='权限', height=1, width=5)
权限.grid(row=0, column=3)

事件f=Frame()
事件f.grid(row=1, column=0)


root.mainloop()
