# parser

本程式可以檢查一個 P 語言程式是否符合語法，還有檢查重複定義的名稱

## 功能

1. 可以用 `//&S+` 列出每一行程式碼，`//&S-` 關閉此功能。預設為列出程式碼
2. 可以用 `//&T+` 列出 token，`//&T-` 關閉此功能。預設為列出 token
3. 可以判斷一個 P 語言程式是否符合語法。如果不符合語法，就會輸出錯誤的位置和錯誤 token
4. 判斷是否有重複定義的變數、常數、或是函數名，只要有錯誤，就會輸出錯誤的位置還有重複的名稱
5. 在每個 `begin ... end` 區塊結束後，顯示這個區塊的符號表的內容。可用 `//&D+` 註解開啟此功能，`//&D-` 關閉，預設是開啟

## 新增檔案

### ast.h
定義字面常數和資料型別的資料結構。字面常數是 `struct Constant`，資料型別則是 `struct Type`

因為未來有可能用到語法樹，所以檔案稱為 ast (Abstract Syntax Tree)

### ast.c
提供複製語法結構、釋放結構的記憶體、以及顯示語法結構的方法

### MyHash.h 和 MyHash.c
我寫的雜湊表實作，符號表會用到

### symtable.h
宣告符號表的結構，還有 attribute 的結構。符號表可用的操作也在這裡宣告

### symtable.c
符號表的實作，我使用雜湊表作為查詢符號用，另外再加上一個堆疊，用來做 scope 操作

指令的功能：
* `initSymTable`：使用符號表前必須先初始化

* `startVarDecl` 和 `startParamDecl`：分別標示變數和參數宣告的開始。實作其實一樣，就只是把目前堆疊的頂端位置存起來

* `addSymbol`：新增一個符號，然後把符號放到堆疊和雜湊表裡。如果成功，就傳回 1，否則傳回 0
* `addLoopSymbol`：和 `addSymbol` 很像，只是加入的符號是 for 迴圈的變數。for 迴圈變數的 scope level 比其他符號多 1
* `removeLoopVar`：移除 1 個 for 迴圈變數

* `endVarDecl`：標示變數宣告的結束。會從上次 `startVarDecl` 儲存的堆疊位置開始，到目前堆疊頂為止，把堆疊上的每個符號都改成變數，並儲存型別資訊
* `endConstDecl`：標示常數宣告的結束。和 `endVarDecl` 很像，但儲存的資訊多了常數值，並把符號改成常數
* `endParamDecl`：標示參數宣告的結束。和 `endVarDecl` 很像，但是把符號改成參數
* `endFuncDecl`：表示一個函數的參數和傳回型別都讀完了。如果函數的符號建立成功的話，這時會從堆疊的頂端往下找第一個不是參數的符號，這個符號只可能是函數。接著會把參數的型別和傳回型別加到函數的屬性

* `pushScope`：進入內層的變數作用區塊，在 P 語言裡，這是指 begin ... end 區塊
* `popScope`：離開內層的變數作用區塊，同時，如果有設定 `//&D+`，就會印出符號表。實作方法就是從堆疊頂開始刪除 scope level = 目前作用域的符號，直到 scope level 變小或是遇到迴圈變數 (因為迴圈變數有不同的 scope rule)

* `destroyAttribute`：安全的銷毀屬性結構，才不會記憶體外洩
* `showAttribute`：顯示符號的屬性
## 對上次作業修改的地方

### scanner (tokens.l)
* 導入自己的標頭檔 `ast.h`，裡面定義字面常數 (literal constant) 的資料結構
* 加入選項 `Opt_D`，控制是否輸出符號表
* 限制標識符 (identifier) 的長度到 32 個字，超過的部份就忽略
* 讀取整數、浮點數、字串時，會把解析的結果存到 `yylval`，讓 parser 可以使用
* 我用 C 語言的 `sscanf` 解析浮點數

### parser (parser.y)
* 導入自己的標頭檔 `symtable.h`，就可以在解析語法的時候操作符號表
* 用 `extern int Opt_D;` 存取 scanner 的輸出符號表選項
* 用 `%union` 定義屬性 (attribute) 的型別。目前屬性可以是名稱、型別或字面常數
* 移除 `UMINUS` 運算符，因為負號運算子的語法現在是非終端符號 `minus_expr`
* 用 `%type<...>` 和 `%token<...>` 為字面常數加上屬性的型別

* 非終端符號 `programname` 和 `functionname` 會把名稱加到符號表
* 非終端符號 `program` 在結尾處會 `popScope`，把全域變數、常數、函數的符號表視為另一個 scope，這樣就能印出全域變數的符號表了
* 宣告變數和常數的語法，因為有相同開頭的關係，所以我加入一個非終端符號 `startVarDecl`，只包含一個動作`startVarDecl()` ，就是向 symtable 通知要開始變數宣告。在確定是變數還是常數宣告後，會呼叫 `endVarDecl` 或 `endConstDecl`，symtable 就會相應反應
* 非終端符號 `identifier_list` 會把名稱加到符號表。不知道符號是什麼種類沒關係，因為使用到 `identifier_list` 的規則會把符號轉成適當的類型

* 非終端符號 `function_decl` 會在讀到參數的時候 `pushScope`，讀完參數後 `endFuncDecl` 表示函數的參數格式讀完了，並指定函數的傳回型別
* 非終端符號 `function_type` 、 `type` 和 `scalar_type` 會輸出型別資訊。`scalar_type` 輸出的是列舉常數，而 `function_type` 、 `type` 輸出 `struct Type *`
* 移除非終端符號 `function_body`，因為它就是非終端符號 `compound_stmt`
* 非終端符號 `formal_arg` 在開始時會用 `startParamDecl()` 通知 symtable 要開始參數宣告。結束時用 `endParamDecl` 通知 symtable 參數的型別

* 原本打算在非終端符號 `compound_stmt` 裡面 `pushScope` 和 `popScope`，但是規格說 function 和 function body 的 compound statement 是同一個 scope，導致我不能在 `compound_stmt` 裡面 `pushScope`。我要在其他用到 `compound_stmt` 的地方加上`{pushScope();}`
* 非終端符號 `for_stmt` 會新增迴圈變數，然後在離開迴圈的時候從符號表移除變數
* 非終端符號 `literal_constant` 和 `integer_constant` 可以處理負數了

* 程式裡有一些奇怪的 `free(...)`，那是為了應付 valgrind 用的，因為目前還沒處理大部分的語法，所以有幾個地方會拋棄 scanner 製造的字串，然後就發生記憶體外洩了。即使用了 free ，我在測試的時候還是看得見 memory leak

## 執行平台

Ubuntu 17.10 和系計中的 Linux 主機 (我只測試過這兩個)

## 執行方法

首先，你需要安裝 lex 和 yacc。可以用 flex 取代 lex，bison 取代 yacc。

然後進入這個資料夾，在命令列輸入 `make`，就可以編譯這個程式

這個程式的命令列格式是 `./parser <檔案名>`

執行後，如果檔案是語法正確的 P 程式，就會顯示 `There is no syntactic error!`

要清除編譯中間檔，請在命令列輸入 `make clean`
