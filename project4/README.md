# parser

本程式可以檢查一個 P 語言程式是否符合語法，還有檢查重複定義的名稱

## 功能

1. 可以用 `//&S+` 列出每一行程式碼，`//&S-` 關閉此功能。預設為列出程式碼
2. 可以用 `//&T+` 列出 token，`//&T-` 關閉此功能。預設為列出 token
3. 可以判斷一個 P 語言程式是否符合語法。如果不符合語法，就會輸出錯誤的位置和錯誤 token
4. 判斷是否有重複定義的變數、常數、或是函數名，只要有錯誤，就會輸出錯誤的位置還有重複的名稱
5. 在每個 `begin ... end` 區塊結束後，顯示這個區塊的符號表的內容。可用 `//&D+` 註解開啟此功能，`//&D-` 關閉，預設是開啟

## 新增檔案
有 errReport.c、errReport.h、semcheck.c 和 semcheck.h

### errReport.c 和 errReport.h
用來輸出語意錯誤的訊息，並計算語意錯誤的數量

### semcheck.c 和 semcheck.h
在 parser 建立語法樹之後，檢查運算式型別

## 對上次作業修改的地方

### scanner (tokens.l)
都是為了除錯
1. `else` 關鍵字的回報改成 `<KWelse>` 了
2. 在所有的字串處理程式都加上邊界檢查

### parser (parser.y)
1. 在語法規則的 action 檢查程式名和函數名
2. 把 expr 拆成好幾個非終端符，好讓我能比較方便的分析型別
3. 在解析運算式的時候會建立語法樹
4. 把函數型別存進 `funcReturnType`，就能檢查 return 型別了
5. 建立語法樹之後就會檢查型別
6. 為了收集到函數的引數，我把 `arg_list` 和 `arguments` 改成帶有屬性，屬性是一個 linked list
7. 處理 `if` 的型別時，為了避免 confilct，我把 `boolean_expr` 非終端符改成 `condition`，然後在 `condition` 規則內檢查 `boolean_expr` 是否是 `boolean` 型別
8. 如果語法正確，而且沒有語法錯誤，則輸出訊息

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
