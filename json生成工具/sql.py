import sqlite3,time
db=sqlite3.connect(r'D:\Program\酷Q\data\47816140\logv1.db')
cursor = db.cursor()
sql = "SELECT * FROM log where name='[↓]私聊消息(好友)'"

# 执行SQL语句
cursor.execute(sql)
# 获取所有记录列表
results = cursor.fetchall()
print("results:")
for row in results:
    timeArray = time.localtime(int(row[1]))
    styleTime = time.strftime("%Y-%m-%d %H:%M:%S", timeArray)
    print(styleTime,row[5],"\n")

# 关闭数据库连接
db.close()