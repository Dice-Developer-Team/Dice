import json, re, chardet

CQevent = {
    "EVE_Startup": {"name": "酷Q启动", "type": 1001},
    "EVE_Exit": {"name": "酷Q退出", "type": 1002},
    "EVE_Enable": {"name": "应用已被启用", "type": 1003},
    "EVE_Disable": {"name": "应用将被停用", "type": 1004},
    "EVE_PrivateMsg": {"name": "私聊消息", "type": 21},
    "EVE_PrivateMsg_EX": {"name": "私聊消息", "type": 21},
    "EVE_GroupMsg": {"name": "群消息", "type": 2},
    "EVE_GroupMsg_EX": {"name": "群消息", "type": 2},
    "EVE_DiscussMsg": {"name": "讨论组消息", "type": 4},
    "EVE_DiscussMsg_EX": {"name": "讨论组消息", "type": 4},
    "EVE_GroupUpload": {"name": "群文件上传事件", "type": 11},
    "EVE_System_GroupAdmin": {"name": "群事件-管理员变动", "type": 101},
    "EVE_System_GroupMemberDecrease": {"name": "群事件-群成员减少", "type": 102},
    "EVE_System_GroupMemberIncrease": {"name": "群事件-群成员增加", "type": 103},
    "EVE_Friend_Add": {"name": "好友事件-好友已添加", "type": 201},
    "EVE_Friend_Add_EX": {"name": "好友事件-好友已添加", "type": 201},
    "EVE_Request_AddFriend": {"name": "请求-好友添加", "type": 301},
    "EVE_Request_AddFriend_EX": {"name": "请求-好友添加", "type": 301},
    "EVE_Request_AddGroup": {"name": "请求-群添加", "type": 302},
    "EVE_Request_AddGroup_EX": {"name": "请求-群添加", "type": 302}
}
CQauth = {
    "getCsrfToken": {"auth": 20, "note": "[敏感]取Cookies"},
    "getCookies": {"auth": 20, "note": "[敏感]取Cookies"},
    "CQ_getCsrfToken": {"auth": 20, "note": "[敏感]取Cookies"},
    "CQ_getCookies": {"auth": 20, "note": "[敏感]取Cookies"},
    "getRecord": {"auth": 30, "note": "接收语音"},
    "CQ_getRecord": {"auth": 30, "note": "接收语音"},
    "sendGroupMsg": {"auth": 101, "note": "发送群消息"},
    "CQ_sendGroupMsg": {"auth": 101, "note": "发送群消息"},
    "sendDiscussMsg": {"auth": 103, "note": "发送讨论组消息"},
    "CQ_sendDiscussMsg": {"auth": 103, "note": "发送讨论组消息"},
    "sendPrivateMsg": {"auth": 106, "note": "发送私聊消息"},
    "CQ_sendPrivateMsg": {"auth": 106, "note": "发送私聊消息"},
    "sendLike": {"auth": 110, "note": "发送赞"},
    "CQ_sendLike": {"auth": 110, "note": "发送赞"},
    "setGroupKick": {"auth": 120, "note": "置群员移除"},
    "CQ_setGroupKick": {"auth": 120, "note": "置群员移除"},
    "setGroupBan": {"auth": 121, "note": "置群员禁言"},
    "CQ_setGroupBan": {"auth": 121, "note": "置群员禁言"},
    "setGroupAdmin": {"auth": 122, "note": "置群管理员"},
    "CQ_setGroupAdmin": {"auth": 122, "note": "置群管理员"},
    "setGroupWholeBan": {"auth": 123, "note": "置全群禁言"},
    "CQ_setGroupWholeBan": {"auth": 123, "note": "置全群禁言"},
    "setGroupAnonymousBan": {"auth": 124, "note": "置匿名群员禁言"},
    "CQ_setGroupAnonymousBan": {"auth": 124, "note": "置匿名群员禁言"},
    "setGroupAnonymous": {"auth": 125, "note": "置群匿名设置"},
    "CQ_setGroupAnonymous": {"auth": 125, "note": "置群匿名设置"},
    "setGroupCard": {"auth": 126, "note": "置群成员名片"},
    "CQ_setGroupCard": {"auth": 126, "note": "置群成员名片"},
    "setGroupLeave": {"auth": 127, "note": "[敏感]置群退出"},
    "CQ_setGroupLeave": {"auth": 127, "note": "[敏感]置群退出"},
    "setGroupSpecialTitle": {"auth": 128, "note": "置群成员专属头衔"},
    "CQ_setGroupSpecialTitle": {"auth": 128, "note": "置群成员专属头衔"},
    "getGroupMemberInfo": {"auth": 130, "note": "取群成员信息"},
    "getGroupMemberInfoV2": {"auth": 130, "note": "取群成员信息"},
    "CQ_getGroupMemberInfo": {"auth": 130, "note": "取群成员信息"},
    "CQ_getGroupMemberInfoV2": {"auth": 130, "note": "取群成员信息"},
    "getStrangerInfo": {"auth": 131, "note": "取陌生人信息"},
    "CQ_getStrangerInfo": {"auth": 131, "note": "取陌生人信息"},
    "setDiscussLeave": {"auth": 140, "note": "置讨论组退出"},
    "CQ_setDiscussLeave": {"auth": 140, "note": "置讨论组退出"},
    "setFriendAddRequest": {"auth": 150, "note": "置好友添加请求"},
    "CQ_setFriendAddRequest": {"auth": 150, "note": "置好友添加请求"},
    "setGroupAddRequest": {"auth": 151, "note": "置群添加请求"},
    "CQ_setGroupAddRequest": {"auth": 151, "note": "置群添加请求"},
    "getGroupMemberList": {"auth": 160, "note": "取群成员列表"},
    "CQ_getGroupMemberList": {"auth": 160, "note": "取群成员列表"},
    "getGroupList": {"auth": 161, "note": "取群列表"},
    "CQ_getGroupList": {"auth": 161, "note": "取群列表"}
}
# 初始化event正则
for key in CQevent.keys():
    pattern = re.compile(r'\b' + key + r'\s*\(\s*(?P<function>\w+)\s*\)')
    CQevent[key]["pattern"] = pattern
# 初始化menu正则
menuPattern = re.compile(r'\bEVE_Menu\s*\(\s*(?P<function>\w+)\s*\)')
# 初始化status正则
statusPattern = re.compile(r'\bEVE_Status[_EX]?\s*\(\s*(?P<function>\w+)\s*\)')
# 初始化auth正则
for key in CQauth.keys():
    pattern = re.compile(r'\b' + key + r'\s*\(')
    CQauth[key]["pattern"] = pattern


def 分析cpp(file, CQjson={}):
    with open(file, "rb") as f:
        try:
            data = f.read()
            text = data.decode(chardet.detect(data)['encoding'])

            if not "event" in CQjson:
                CQjson["event"] = []
            eventFind(text, CQjson["event"])

            if not "menu" in CQjson:
                CQjson["menu"] = []
            menuFind(text, CQjson["menu"])

            if not "status" in CQjson:
                CQjson["status"] = []
            statusFind(text, CQjson["status"])

            if not "auth" in CQjson:
                CQjson["auth"] = []
            authFind(text, CQjson["auth"])

        except StopIteration:
            pass
        f.close()
    return CQjson


def eventFind(str, list_event=[]):
    for key in CQevent.keys():
        result = re.findall(CQevent[key]["pattern"], str)
        if len(result) > 0:
            for function in result:
                if not hasfunction(list_event, function):
                    list_event.append({
                        "name": CQevent[key]["name"],
                        "type": CQevent[key]["type"],
                        "function": function
                    })
    return list_event


def menuFind(str, list_menu=[]):
    result = re.findall(menuPattern, str)
    if len(result) > 0:
        for menu in result:
            if not hasfunction(list_menu, menu):
                list_menu.append({
                    "name": menu,
                    "function": menu
                })
    return list_menu


def statusFind(str, list_status=[]):
    result = re.findall(statusPattern, str)
    if len(result) > 0:
        for status in result:
            if not hasfunction(list_status, status):
                list_status.append({
                    "name": status,
                    "title": status,
                    "function": status,
                    "period": "1000"
                })
    return list_status


def authFind(str, set_auth=[]):
    authset = set(set_auth)
    for key in CQauth:
        authinfo = CQauth[key]
        if re.search(authinfo["pattern"], str):
            auth = authinfo["auth"]
            if not auth in authset:
                set_auth.append(auth)
                authset.add(auth)
    return set_auth


def hasfunction(CQjsonList, functionName):
    for eve in CQjsonList:
        if eve["function"] == functionName:
            return True
    return False


def 分析多个CPP(files, CQjson={}):
    for file in files:
        if __name__ == "__main__":
            print(file)
        分析cpp(file, CQjson)
    return CQjson


if __name__ == "__main__":
    import 文件搜索

    CQjson = 分析多个CPP(文件搜索.listCppInDir(r"D:\MyCode\CPP\CQ\CQPSDK\src"))
    j1 = json.dumps(CQjson, indent=4, ensure_ascii=False)
    print(j1)
