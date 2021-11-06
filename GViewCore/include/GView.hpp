#include <AppCUI/include/AppCUI.hpp>

using namespace AppCUI::Controls;
using namespace AppCUI::Utils;
using namespace AppCUI::Graphics;

#ifdef CORE_EXPORTABLE
#    ifdef BUILD_FOR_WINDOWS
#        define CORE_EXPORT __declspec(dllexport)
#    else
#        define CORE_EXPORT
#    endif
#else
#    define CORE_EXPORT
#endif

#ifdef BUILD_FOR_WINDOWS
#    define PLUGIN_EXPORT __declspec(dllexport)
#else
#    define PLUGIN_EXPORT
#endif

namespace GView
{
namespace Utils
{
    typedef void* Instance;
    constexpr unsigned long long INVALID_OFFSET = 0xFFFFFFFFFFFFFFFFULL;
    constexpr int INVALID_SELECTION_INDEX       = -1;

    struct Buffer
    {
        const unsigned char* data;
        const unsigned int length;
        Buffer() : data(nullptr), length(0)
        {
        }
        Buffer(const unsigned char* d, unsigned int l) : data(d), length(l)
        {
        }
        constexpr inline bool Empty() const
        {
            return length == 0;
        }
        constexpr inline unsigned char operator[](unsigned int index) const
        {
            return *(data + index);
        }
    };
    class CORE_EXPORT FileCache
    {
        AppCUI::OS::IFile* fileObj;
        unsigned long long fileSize, start, end, currentPos;
        unsigned char* cache;
        unsigned int cacheSize;

      public:
        FileCache();
        ~FileCache();

        bool Init(std::unique_ptr<AppCUI::OS::IFile> file, unsigned int cacheSize);
        Buffer Get(unsigned long long offset, unsigned int requestedSize);
        bool Copy(void* buffer, unsigned long long offset, unsigned int requestedSize);
        inline unsigned char GetFromCache(unsigned long long offset, unsigned char defaultValue = 0) const
        {
            if ((offset >= start) && (offset < end))
                return cache[offset - start];
            return defaultValue;
        }
        inline Buffer Get(unsigned int requestedSize)
        {
            return Get(currentPos, requestedSize);
        }

        inline unsigned long long GetSize() const
        {
            return fileSize;
        }
        inline unsigned long long GetCurrentPos() const
        {
            return currentPos;
        }
        inline void SetCurrentPos(unsigned long long value)
        {
            currentPos = value;
        }

        template <typename T>
        inline bool Copy(unsigned long long offset, T& object)
        {
            return Copy(&object, offset, sizeof(T));
        }
    };

} // namespace Utils
struct CORE_EXPORT Object
{
    Utils::FileCache cache;
    Utils::Instance instance;
    Object() : instance(nullptr)
    {
    }
};

namespace View
{
    typedef unsigned char MethodID;
    struct CORE_EXPORT ViewControl : public AppCUI::Controls::UserControl
    {
        virtual bool GoTo(unsigned long long offset)                                                                       = 0;
        virtual bool Select(unsigned long long offset, unsigned long long size)                                            = 0;
        virtual std::string_view GetName()                                                                                 = 0;
        virtual void PaintCursorInformation(AppCUI::Graphics::Renderer& renderer, unsigned int width, unsigned int height) = 0;

        ViewControl() : UserControl("d:c")
        {
        }
    };
    struct CORE_EXPORT BufferViewInterface
    {
        virtual void AddZone(unsigned long long start, unsigned long long size, ColorPair col, std::string_view name) = 0;
        virtual void AddBookmark(unsigned char bookmarkID, unsigned long long fileOffset)                             = 0;
        virtual void AddOffsetTranslationMethod(std::string_view name, MethodID methodID)                             = 0;
    };
    struct CORE_EXPORT WindowInterface
    {
        virtual Reference<Object> GetObject()                                              = 0;
        virtual bool AddPanel(Pointer<TabPage> page, bool vertical)                        = 0;
        virtual Reference<BufferViewInterface> AddBufferView(const std::string_view& name) = 0;
        virtual Reference<ViewControl> GetCurrentView()                                    = 0;
    };
}; // namespace View
EXPORT void Nothing();
}; // namespace GView
