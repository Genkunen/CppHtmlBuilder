#include <array>
#include <string>
#include <utility>


template<typename T, typename = void>
struct is_iterable : std::false_type {};

template<typename T>
struct is_iterable<T,
  std::void_t<decltype(begin(std::declval<T&>()) != end(std::declval<T&>()),
    ++std::declval<decltype(begin(std::declval<T&>()))&>(),
    void(*begin(std::declval<T&>())))>> : std::true_type {};


enum HtmlElementFlags : uint8_t {
    VOID_TAG = 1,
    ATTR_TAG = 2,
};

template<size_t Size = 0, uint8_t Flags = 0>
class HtmlElement {
private:
    static const bool IsVoidTag = static_cast<bool>(Flags & VOID_TAG);
    static const bool HasAttrs = static_cast<bool>(Flags & ATTR_TAG);
    // HasAttrs ? additional space between tag name and attrs : no attrs
    // IsVoidTag counts in null terminator regardless of the result
    static const size_t AdditionalSize = (IsVoidTag ? sizeof("< />") : sizeof("<></>")) + static_cast<int>(HasAttrs);
    struct has_attr : std::bool_constant<HasAttrs> {};

public:
    // Size + 1 -> adding back the missing null terminator from deduction guide
    constexpr explicit HtmlElement(const char (&tag)[Size + 1U]) {
        copyInto(m_data, "<", tag, " />");
        //
    }

    template<size_t Size1, size_t Size2>
    constexpr explicit HtmlElement(const char (&str1)[Size1], const char (&str2)[Size2])
      : HtmlElement(str1, str2, has_attr{}) {}

    [[nodiscard]] constexpr auto c_str() const noexcept { return m_data; }
    [[nodiscard]] constexpr auto size() const noexcept(noexcept(std::size(m_data))) { return std::size(m_data); }

    template<size_t Attr_Size>
    constexpr auto operator[](const char (&attr)[Attr_Size]) const {
        return SetAttr<Size + AdditionalSize, Attr_Size, Flags>{ m_data, attr };
    }
    int test{};

private:
    template<size_t Tag_Size, size_t Content_Size>
    constexpr explicit HtmlElement(const char (&tag)[Tag_Size], const char (&content)[Content_Size], std::false_type) {
        copyInto(m_data, "<", tag, ">", content, "</", tag, ">");
    }

    template<size_t Data_Size, size_t Attr_Size>
    constexpr explicit HtmlElement(const char (&data)[Data_Size], const char (&attr)[Attr_Size], std::true_type) {
        if constexpr (IsVoidTag) {
            test = Size + AdditionalSize;
            size_t next{};
            size_t resume{};
            while (data[next] != '/') {
                m_data[next] = data[next];
                next++;
            }
            resume = next;
            for (size_t i = 0; i < Attr_Size - 1; ++i) {
                m_data[next++] = attr[i];
            }
            while (data[resume]) {
                m_data[next++] = data[resume++];
            }
        }
        else {
            test = Size + AdditionalSize;
            size_t next{};
            size_t resume{};
            while (data[next] != '>') {
                m_data[next] = data[next];
                next++;
            }
            resume = next;
            m_data[next++] = ' ';
            for (size_t i = 0; i < Attr_Size - 1; ++i) {
                m_data[next++] = attr[i];
            }
            while (data[resume]) {
                m_data[next++] = data[resume++];
            }
        }
    }

    template<size_t Data_Size, size_t Attr_Size, uint8_t Flags_>
    struct SetAttr {
        constexpr explicit SetAttr(const char (&data)[Data_Size], const char (&attr)[Attr_Size])
          : m_data{ data }, m_attr{ attr } {}

        ~SetAttr() = default;

        constexpr SetAttr(const SetAttr&) = delete;
        constexpr SetAttr(SetAttr&&) = delete;
        auto operator=(const SetAttr&) -> SetAttr& = delete;
        auto operator=(SetAttr&&) -> SetAttr& = delete;

        template<size_t Attr_Content_Size>
        constexpr auto operator()(const char (&attrContent)[Attr_Content_Size]) const {
            // -2 for attrContent's null terminator and sizeof's null terminator
            char concat[Attr_Size + Attr_Content_Size + sizeof("=\"\"") - 2]{};
            copyInto(concat, m_attr, "=\"", attrContent, "\"");
            return HtmlElement<Data_Size + Attr_Size + Attr_Content_Size + sizeof("=\"\"") - 3, Flags_ | ATTR_TAG>{
                m_data, concat
            };
        }

    private:
        // SetAttr is both non-copyable and non-movable, so cref data members are ok
        const char (&m_data)[Data_Size];
        const char (&m_attr)[Attr_Size];
    };

    template<typename Dest, typename... Strs>
    static constexpr void copyInto(Dest& dest, Strs&... strs) {
        size_t next{};
        (
          [&] {
              for (size_t i = 0; strs[i]; ++i) {
                  dest[next++] = strs[i];
              }
          }(),
          ...);
    };
    char m_data[Size + AdditionalSize]{};
};

template<size_t Size>
explicit HtmlElement(const char (&)[Size]) -> HtmlElement<Size - 1, VOID_TAG>;

template<size_t Tag_Size, size_t Content_Size>
explicit HtmlElement(const char (&)[Tag_Size],
  const char (&)[Content_Size]) -> HtmlElement<((Tag_Size - 1) * 2) + Content_Size - 1>;

template<>
class HtmlElement<0, 0> {
public:
    template<typename T>
    explicit HtmlElement(T t) : HtmlElement(t, is_iterable<T>{}) {}

    [[nodiscard]] auto c_str() const { return m_data; }

private:
    template<typename T>
    HtmlElement(T t, [[maybe_unused]] std::true_type _) : m_data("iterable") {
        (void)t;
    }

    template<typename T>
    HtmlElement(T t, [[maybe_unused]] std::false_type _) : m_data("not iterable") {
        (void)t;
    }

    std::string m_data;
};

template<typename T>
HtmlElement(T) -> HtmlElement<0, 0>;