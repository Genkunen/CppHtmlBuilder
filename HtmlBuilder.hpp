#include <array>
#include <string>
#include <type_traits>
#include <utility>

template<typename T, typename = void>
struct is_iterable : std::false_type {};

// constexpr version
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
    static constexpr bool IsVoidTag = static_cast<bool>(Flags & VOID_TAG);
    static constexpr bool HasAttrs = static_cast<bool>(Flags & ATTR_TAG);
    // HasAttrs ? additional space between tag name and attrs : no attrs
    static constexpr size_t AdditionalSize = []() -> size_t {
        if constexpr (HasAttrs) {
            return static_cast<size_t>(!IsVoidTag);
        }
        return (IsVoidTag ? sizeof("< />") : sizeof("<></>"));
    }();

    struct has_attr : std::bool_constant<HasAttrs> {};

public:
    static constexpr auto SIZE = Size + AdditionalSize;

    // Size + 1 -> adding back the missing null terminator from deduction guide
    constexpr explicit HtmlElement(const char (&tag)[Size + 1U]) {
        copyInto(m_data, "<", tag, " />");
    }

    template<size_t Size1, size_t Size2, size_t... Sizes>
    constexpr explicit HtmlElement(const char (&str1)[Size1], const char (&str2)[Size2], const char (&... more)[Sizes])
      : HtmlElement(str1, str2, has_attr{}, more...) {}

    template<size_t Size1, size_t... Sizes, uint8_t... Flags_>
    constexpr explicit HtmlElement(const char (&tag)[Size1], const HtmlElement<Sizes, Flags_>&... elements) {
        copyInto(m_data, "<", tag, ">", elements.data()..., "</", tag, ">");
    }

    template<typename T, bool>
    struct get_size : std::integral_constant<int, sizeof(T) - 1> {};
    template<typename T>
    struct get_size<T, false> : std::integral_constant<int, T::SIZE - 1> {};

    template<size_t Content_Size, class... Elements>
    constexpr auto operator()(const char (&content)[Content_Size], const Elements&... elements) const {
        static_assert(Flags == VOID_TAG);
        constexpr size_t TAG_SIZE = SIZE - sizeof("< />");
        char tag[TAG_SIZE + 1]{};
        for (size_t i = 0; i < TAG_SIZE; ++i) {
            tag[i] = m_data[i + 1];
        }
        constexpr size_t ADDITIONAL =
          ([&] { return get_size<Elements, std::is_array_v<Elements>>::value; }() + ... + 0);
        return HtmlElement<((TAG_SIZE) * 2) + Content_Size - 1 + ADDITIONAL, 0>{
            tag, content, Unwind(std::move(elements))...
        };
    }

    template<class... Elements>
    constexpr auto operator()(const Elements&... elements) const {
        return operator()("", elements...);
    }

    template<size_t Attr_Size>
    constexpr auto operator[](const char (&attr)[Attr_Size]) const {
        return SetAttr<Size + AdditionalSize, Attr_Size, Flags>{ m_data, attr };
    }

    [[nodiscard]] constexpr auto data() const noexcept -> const char (&)[Size + AdditionalSize] { return m_data; }
    [[nodiscard]] constexpr auto size() const noexcept(noexcept(std::size(m_data))) -> size_t {
        return std::size(m_data);
    }

private:
    template<size_t Size1>
    constexpr auto Unwind(const char (&&str)[Size1]) const -> const char (&)[Size1] {
        return std::move(str);
    }

    template<size_t Size1, uint8_t Flags_>
    constexpr auto Unwind(const HtmlElement<Size1, Flags_>& h) const -> const
      char (&)[HtmlElement<Size1, Flags_>::SIZE] {
        return h.data();
    }

    template<size_t Tag_Size, size_t Content_Size, size_t... Sizes>
    constexpr explicit HtmlElement(const char (&tag)[Tag_Size],
      const char (&content)[Content_Size],
      [[maybe_unused]] std::false_type _,
      const char (&... more)[Sizes]) {
        copyInto(m_data, "<", tag, ">", content, more..., "</", tag, ">");
    }

    template<size_t Data_Size, size_t Attr_Size>
    constexpr explicit HtmlElement(const char (&data)[Data_Size],
      const char (&attr)[Attr_Size],
      [[maybe_unused]] std::true_type _) {
        if constexpr (IsVoidTag) {
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
    static constexpr void copyInto(Dest& dest, const Strs&... strs) {
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

    template<size_t Size1, uint8_t Flags_>
    friend constexpr auto Unwind(const HtmlElement<Size1, Flags_>& h) -> const
      char (&)[HtmlElement<Size1, Flags_>::SIZE];
};

template<size_t Size>
explicit HtmlElement(const char (&)[Size]) -> HtmlElement<Size - 1, VOID_TAG>;

template<size_t Tag_Size, size_t Content_Size, size_t... Sizes>
explicit HtmlElement(const char (&)[Tag_Size], const char (&)[Content_Size], const char (&...)[Sizes])
  -> HtmlElement<((Tag_Size - 1) * 2) + Content_Size - 1 + (Sizes + ... + 0) - sizeof...(Sizes)>;

template<size_t Size1, size_t... Sizes, uint8_t... Flags_>
HtmlElement(const char (&)[Size1], const HtmlElement<Sizes, Flags_>&...)
  -> HtmlElement<((Size1 - 1) * 2) + (HtmlElement<Sizes - 1, Flags_>::SIZE + ... + 0)>;

// run-time version
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