# parser

本程式可以檢查一個 P 語言程式是否符合語法，還有檢查重複定義的名稱

## 功能

1. 可以用 `//&S+` 列出每一行程式碼，`//&S-` 關閉此功能。預設為列出程式碼
2. 可以用 `//&T+` 列出 token，`//&T-` 關閉此功能。預設為列出 token
3. 可以判斷一個 P 語言程式是否符合語法。如果不符合語法，就會輸出錯誤的位置和錯誤 token
4. 判斷是否有重複定義的變數、常數、或是函數名，只要有錯誤，就會輸出錯誤的位置還有重複的名稱
5. 在每個 `begin ... end` 區塊結束後，顯示這個區塊的符號表的內容。可用 `//&D+` 註解開啟此功能，`//&D-` 關閉，預設是開啟

## 新增檔案

## 對上次作業修改的地方

### scanner (tokens.l)

### parser (parser.y)

## 執行平台

Ubuntu 17.10 和系計中的 Linux 主機 (我只測試過這兩個)

## 執行方法

首先，你需要安裝 flex 和 bison。

然後進入這個資料夾，在命令列輸入 `make`，就可以編譯這個程式

這個程式的命令列格式是 `./parser <檔案名>`

執行後，如果檔案是語法正確的 P 程式，就會顯示 `There is no syntactic and semantic error!`

要清除編譯中間檔，請在命令列輸入 `make clean`