
#include <wx/defs.h>
#include <wx/imaglist.h>


/* C::B begin */
#ifndef wxOVERRIDE
    #define wxOVERRIDE override
#endif // wxOVERRIDE
#ifndef wxFALLTHROUGH
    #define wxFALLTHROUGH
#endif // wxFALLTHROUGH
/* C::B end */

wxRect wxRectFromPRectangle(PRectangle prc);
PRectangle PRectangleFromwxRect(wxRect rc);
wxColour wxColourFromCD(const ColourDesired & ca);

class ListBoxImpl : public ListBox
{
    private:
        int                 lineHeight;
        bool                unicodeMode;
        int                 desiredVisibleRows;
        int                 aveCharWidth;
        size_t              maxStrWidth;
        Point               location;       // Caret location at which the list is opened
        wxImageList    *    imgList;
        wxArrayInt     *    imgTypeMap;
        /* C::B begin */
        int                 technology;
        /* C::B end */

    public:
        ListBoxImpl();
        ~ListBoxImpl();
        static ListBox * Allocate();

        virtual void SetFont(Font & font) wxOVERRIDE;
        virtual void Create(Window & parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, int technology_) wxOVERRIDE;
        virtual void SetAverageCharWidth(int width) wxOVERRIDE;
        virtual void SetVisibleRows(int rows) wxOVERRIDE;
        virtual int GetVisibleRows() const wxOVERRIDE;
        virtual PRectangle GetDesiredRect() wxOVERRIDE;
        virtual int CaretFromEdge() wxOVERRIDE;
        virtual void Clear() wxOVERRIDE;
        virtual void Append(char * s, int type = -1) wxOVERRIDE;
        void Append(const wxString & text, int type);
        virtual int Length() wxOVERRIDE;
        virtual void Select(int n) wxOVERRIDE;
        virtual int GetSelection() wxOVERRIDE;
        virtual int Find(const char * prefix) wxOVERRIDE;
        virtual void GetValue(int n, char * value, int len) wxOVERRIDE;
        virtual void RegisterImage(int type, const char * xpm_data) wxOVERRIDE;
        void RegisterImageHelper(int type, const wxBitmap & bmp);
        virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char * pixelsImage) wxOVERRIDE;
        virtual void ClearRegisteredImages() wxOVERRIDE;
        virtual void SetDoubleClickAction(CallBackAction, void *) wxOVERRIDE;
        virtual void SetList(const char * list, char separator, char typesep) wxOVERRIDE;
};

class SurfaceData
{
    public:
        virtual ~SurfaceData() {}
};

#if wxUSE_GRAPHICS_DIRECT2D

#include "wx/msw/private/graphicsd2d.h"
#include "wx/msw/private/comptr.h"

class ScintillaWX;

class SurfaceDataD2D: public SurfaceData
{
    public:
        SurfaceDataD2D(ScintillaWX *);
        bool Initialised() const;
        void DiscardGraphicsResources();
        HRESULT CreateGraphicsResources();
        void SetEditorPaintAbandoned();

        ID2D1DCRenderTarget * GetRenderTarget() const
        {
            return m_pRenderTarget.get();
        }
        ID2D1SolidColorBrush * GetSolidBrush() const
        {
            return m_pSolidBrush.get();
        }
        ID2D1BitmapBrush * GetPatternBrush() const
        {
            return m_pPatternBrush.get();
        }
        IDWriteRenderingParams * GetDefaultRenderingParams() const
        {
            return m_defaultRenderingParams;
        }
        IDWriteRenderingParams * GetCustomClearTypeRenderingParams() const
        {
            return m_customClearTypeRenderingParams;
        }

    private:
        wxCOMPtr<ID2D1Factory> m_pD2DFactory;
        wxCOMPtr<IDWriteFactory> m_pDWriteFactory;
        wxCOMPtr<ID2D1DCRenderTarget> m_pRenderTarget;
        wxCOMPtr<ID2D1SolidColorBrush> m_pSolidBrush;
        wxCOMPtr<ID2D1BitmapBrush> m_pPatternBrush;
        wxCOMPtr<IDWriteRenderingParams> m_defaultRenderingParams;
        wxCOMPtr<IDWriteRenderingParams> m_customClearTypeRenderingParams;

        ScintillaWX * m_editor;
};

class SurfaceFontDataD2D: public SurfaceData
{
    public:
        SurfaceFontDataD2D(const FontParameters & fp);
        bool Initialised() const;

        XYPOSITION GetAscent() const
        {
            return m_ascent;
        }
        XYPOSITION GetDescent() const
        {
            return m_descent;
        }
        XYPOSITION GetInternalLeading() const
        {
            return m_internalLeading;
        }
        XYPOSITION GetAverageCharWidth() const
        {
            return m_averageCharWidth;
        }

        D2D1_TEXT_ANTIALIAS_MODE GetFontQuality() const
        {
            return m_aaMode;
        }
        IDWriteTextFormat * GetFormat() const
        {
            return m_pTextFormat.get();
        }

    private:
        XYPOSITION m_ascent;
        XYPOSITION m_descent;
        XYPOSITION m_internalLeading;
        XYPOSITION m_averageCharWidth;
        D2D1_TEXT_ANTIALIAS_MODE m_aaMode;
        wxCOMPtr<IDWriteTextFormat> m_pTextFormat;
};

#endif // wxUSE_GRAPHICS_DIRECT2D
