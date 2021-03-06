# parser

本程式可以檢查一個 P 語言程式是否符合語法

## 功能

1. 可以用 `//&S+` 列出每一行程式碼，`//&S-` 關閉此功能。預設為列出程式碼
2. 可以用 `//&T+` 列出 token，`//&T-` 關閉此功能。預設為列出 token
3. 可以判斷一個 P 語言程式是否符合語法。

    如果不符合語法，就會輸出錯誤的位置和錯誤 token

4. 不會檢查型別，因為這不是 context free 的範疇
5. 遇到錯誤的 token，會顯示 token 發生何種錯誤

## 對上次 scanner 作業修改的地方

1. 在開頭處 `#include "y.tab.h"`，這樣才能使用 yacc parser 裡的 token 名稱
2. 除了空格和註解外，在每個 token 規則的動作都加上 `return <token>;`

## 執行平台

Ubuntu 17.10 和系計中的 Linux 主機 (我只測試過這兩個)

## 執行方法

首先，你需要安裝 lex 和 yacc。可以用 flex 取代 lex，bison 取代 yacc。

然後進入這個資料夾，在命令列輸入 `make`，就可以編譯這個程式

這個程式的命令列格式是 `./parser <檔案名>`

執行後，如果檔案是語法正確的 P 程式，就會顯示 `There is no syntactic error!`

要清除編譯中間檔，請在命令列輸入 `make clean`
