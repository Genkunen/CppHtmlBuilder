# Both fully constexpr and run-time HTML builder
> [!Caution]
> Heavily undeveloped, yet.
## Compile time constants
The class provides with full constexpr support in c++17, allowing to use string-literals and other constexpr char arrays in its creation.
The code below compiles into constexpr char array containing HTML code.
```cpp
constexpr auto page =
HtmlElement(
  "body",
  HtmlElement(
    "weirdtag", 
    "weird text"
  ) ["style"]("color: red;")
);
```
```html
<html>
  <head></head>
  <body>
    <weirdtag style="color: red;">
      weird text
    </weirdtag>
  </body>
</html>
```
## Custom tag creation
HtmlElement has overloaded operator(), which enables functor objects creation.
```cpp
constexpr auto Html = HtmlElement{ "html" };
constexpr auto Head = HtmlElement{ "head" };
constexpr auto Body = HtmlElement{ "body" };
constexpr auto Div = HtmlElement{ "div" };
constexpr auto Br = HtmlElement{ "br" };

constexpr auto page = Html(
  Head, // parentheses are optional (Head and Head() results in the same effect)
  Body(
    "Green text",
    Br,
    "More green text"
  ) ["style"]("color:green;")["id"]("someid")
);
```
```html
<html>
  <head></head>
  <body style="color:green;" id="someid">
    Green text <br> More green text
  </body>
</html>
```
## Roadmap
basic constexpr functionality✅ ⮕ polished constexpr code🔰 ⮕ basic run-time functionality❌ ⮕ ...







