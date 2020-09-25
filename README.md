# Dice!
QQ Dice Robot For TRPG Based on CoolQ/Mirai/XQ

[![License](https://img.shields.io/github/license/Dice-Developer-Team/Dice.svg)](http://www.gnu.org/licenses)
[![Build status](https://ci.appveyor.com/api/projects/status/7uq2qi3348ny1tfv?svg=true)](https://ci.appveyor.com/project/w4123/dice-ovf7o)
[![Downloads](https://img.shields.io/github/downloads/Dice-Developer-Team/dice/total.svg)](https://github.com/Dice-Developer-Team/Dice/releases)
[![GitHub contributors](https://img.shields.io/github/contributors/Dice-Developer-Team/dice.svg)](https://github.com/Dice-Developer-Team/Dice/graphs/contributors)
[![GitHub last commit](https://img.shields.io/github/last-commit/Dice-Developer-Team/dice.svg)](https://github.com/Dice-Developer-Team/Dice/commits)

## 简介

Dice!是一款基于酷Q的QQ跑团掷骰机器人 交流QQ群:882747577或941980833或624807593(已满) 也可在MiraiNative或CQXQ等CQ兼容性况下运行

主页: <https://kokona.tech/>

论坛: <https://forum.kokona.tech>

Latest Stable Release: [![GitHub release](https://img.shields.io/github/release/Dice-Developer-Team/dice.svg)](https://github.com/w4123/Dice-Developer-Team/releases) [![GitHub Release Date](https://img.shields.io/github/release-date/Dice-Developer-Team/dice.svg)](https://github.com/Dice-Developer-Team/Dice/releases)

Latest Release: [![GitHub release](https://img.shields.io/github/release-pre/Dice-Developer-Team/dice.svg)](https://github.com/Dice-Developer-Team/Dice/releases) [![GitHub Release Date](https://img.shields.io/github/release-date-pre/Dice-Developer-Team/dice.svg)](https://github.com/Dice-Developer-Team/Dice/releases)

## 开发者

贡献者:w4123溯洄 Shiki jh123111 緋色月下、スイカを食う

感谢:Flandre Cirno 回転 他是王家乐。白いとう 哞哞哞哞哞哞哞哞哞哞哞哞 丸子 黯星 一盏大师 初音py2001 Coxxs orzFly 等(排名不分先后)(如有缺漏请务必联系溯洄:QQ1840686745) 

## 编译须知

请使用最新版Visual Studio 2019或以上版本 (或其独立编译器)进行编译, 项目主文件为Dice.sln

新增: 现在可以用GCC/Clang编译, 只测试了几个版本, 编译出现问题请反馈, 下面列出编译选项, 正在写cmake

- GCC(9.0.0+): ` g++ -shared -static -std=c++17 -O2 -o com.w4123.dice.dll -Wl,--kill-at -I CQSDK\ -I Dice\ CQSDKCPP\*.cpp Dice\*.cpp Dice\CQP.lib -pthread -lWinInet -luser32 `
- Clang(4+)+MSVC(VS2019+): ` clang-cl --target=i686-pc-windows-msvc /MT /O2 /EHsc /std:c++17 /D "UNICODE" /LD /link "user32.lib" /o com.w4123.dice.dll /I CQSDK\ /I Dice\ CQSDKCPP\*.cpp Dice\*.cpp Dice\CQP.lib -Wno-invalid-source-encoding  `
- Clang(4+)+GCC(9.0.0+): ` clang++ --target=i686-pc-windows-gnu -m32 -shared -static -o com.w4123.dice.dll -Xclang -flto-visibility-public-std -Wl,--kill-at -std=c++17 -O2 -I CQSDK\ -I Dice\ CQSDKCPP\*.cpp Dice\*.cpp Dice\CQP.lib -lWinInet -luser32 -pthread -Wno-invalid-source-encoding  `

编译后会得到com.w4123.dice.dll文件! 请从Releases中下载对应的json文件(或自己编写), 放至酷Q/MN/XQ 对应文件夹下使用

## Issue提交

提交Issue前请务必确认没有其他人提交过相同的Issue, 善用搜索功能 提交Bug时最好附有截图以及问题复现方法, 同时也欢迎新功能建议

## License

Dice! QQ Dice Robot for TRPG

Copyright (C) 2018-2020 w4123溯洄 Shiki

This program is free software: you can redistribute it and/or modify it under the terms
of the GNU Affero General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this
program. If not, see <http://www.gnu.org/licenses/>.

