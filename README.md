# 测试中的扩展功能
Log记录与染色、邮件发送模块、文件抽取功能、喵稣噜属性作成。。

该测试版骰子QQ:1951842966。欢迎大家提出意见和建议。具体指令可.help查看。

基线系统见：https://github.com/w4123/Dice

## 额外配置：
1、需要在系统中安装python2.7.16(可在python官方网站的历史版本中找到，默认安装到C:\Python2786路径。)

2、在VS的解决方案右键->属性->VC++目录->库目录中添加该Python环境的libs目录。(默认为C:\Python2786\libs)

3、将Files目录中的文件放到任意位置，并在代码中搜索"F://"(没有双引号),修改对应的路径。(默认路径为"F://")

# Dice!
QQ Dice Robot For TRPG Based on CoolQ 

[![License](https://img.shields.io/github/license/w4123/Dice.svg)](http://www.gnu.org/licenses)
[![Build status](https://ci.appveyor.com/api/projects/status/6qm1l31k07dst0rk?svg=true)](https://ci.appveyor.com/project/w4123/dice)
[![Doc Build status](https://readthedocs.org/projects/dice-for-qq/badge/?badge=latest)](http://docs.kokona.tech)
[![Downloads](https://img.shields.io/github/downloads/w4123/dice/total.svg)](https://github.com/w4123/Dice/releases)
[![GitHub contributors](https://img.shields.io/github/contributors/w4123/dice.svg)](https://github.com/w4123/Dice/graphs/contributors)
[![GitHub last commit](https://img.shields.io/github/last-commit/w4123/dice.svg)](https://github.com/w4123/Dice/commits)

## 简介

Dice!是一款基于酷Q的QQ跑团掷骰机器人 交流QQ群:941980833或624807593(已满)

GitHub Page: <http://kokona.tech/>

Doc: <http://docs.kokona.tech/>

Latest Stable Release: [![GitHub release](https://img.shields.io/github/release/w4123/dice.svg)](https://github.com/w4123/Dice/releases) [![GitHub Release Date](https://img.shields.io/github/release-date/w4123/dice.svg)](https://github.com/w4123/Dice/releases)

Latest Release: [![GitHub release](https://img.shields.io/github/release-pre/w4123/dice.svg)](https://github.com/w4123/Dice/releases) [![GitHub Release Date](https://img.shields.io/github/release-date-pre/w4123/dice.svg)](https://github.com/w4123/Dice/releases)

## 开发者

贡献者:w4123溯洄 jh123111 緋色月下、スイカを食う

感谢:Flandre Cirno 回転 他是王家乐。白いとう 哞哞哞哞哞哞哞哞哞哞哞哞 丸子 黯星 一盏大师 初音py2001 Coxxs orzFly 等(排名不分先后)(如有缺漏请务必联系溯洄:QQ1840686745) 

## 编译须知

从GitHub克隆源码时请不要直接从master分支克隆, 因为所有的更改都会提交到此分支, 很有可能包含最新的测试性更改, 未经过测试无法保证稳定 请选择Tag中最新的Release进行下载

请使用最新版Visual Studio **2015或2017** (或其独立编译器)进行编译, 项目主文件为Dice.sln, 编译时务必使用Win32模式否则无法编译成功

编译后会得到com.w4123.dice.dll文件, 请勿更改此文件的名称! 请从Releases中下载对应的json文件(或自己编写), 放至酷Q app文件夹下, 并开启开发模式, 在应用管理中合成cpk文件即可正常使用

## Issue提交

提交Issue前请务必确认没有其他人提交过相同的Issue, 善用搜索功能 提交Bug时最好附有截图以及问题复现方法, 同时也欢迎新功能建议

## License

Dice! QQ Dice Robot for TRPG

Copyright (C) 2018-2019 w4123溯洄

This program is free software: you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this
program. If not, see <http://www.gnu.org/licenses/>.

