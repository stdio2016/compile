# parser

本程式可以檢查一個 P 語言程式是否符合語法，還有檢查語意

## 功能

1. print 和 read 敘述
2. 讀取常數
3. 四則運算
4. 布林運算，其中 and 和 or 有用到短路邏輯
5. 字串相連
6. 可以使用陣列和字串變數！
7. 函數的陣列引數採 pass by value

## 新增檔案
有 StringBuffer.c、StringBuffer.h、codegen.c 和 codegen.h

### StringBuffer.c 和 StringBuffer.h
用來輸出語意錯誤的訊息，並計算語意錯誤的數量

### codegen.c 和 codegeb.h
生成 Java assembly 用的程式，提供很多生成 assembly 的方法，還有管理 backpatch。

## 對上次作業修改的地方

### scanner (tokens.l)
在 assembly 輸出加上原始碼註解

### parser (parser.y)
1. 增加 code emit 動作
2. 更改 grammar，以讓我做到 backpatch 和短路邏輯

### symtable.c 和 symtable.h
在 Symbol table 上增加屬性：暫時變數編號

### Makefile
因為有新增檔案，所以就改掉 Makefile 了

## 執行平台

Ubuntu 17.10 和系計中的 Linux 主機 (我只測試過這兩個)

## 執行方法

首先，你需要安裝 flex 和 bison。

然後進入這個資料夾，在命令列輸入 `make`，就可以編譯這個程式

這個程式的命令列格式是 `./parser <檔案名>`

執行後，如果檔案是語法正確的 P 程式，就會顯示 `There is no syntactic and semantic error!`

要清除編譯中間檔，請在命令列輸入 `make clean`
