#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>

class CHFrames;

class CHFPassElement : public IPassElement {
  public:
    struct SHFData {
        CHFrames*         deco = nullptr;
        float             a    = 1.F;
    };

    CHFPassElement(const SHFData& data_);
    virtual ~CHFPassElement() = default;

    virtual void        draw(const CRegion& damage);
    virtual bool        needsLiveBlur();
    virtual bool        needsPrecomputeBlur();

    virtual const char* passName() {
        return "CHFPassElement";
    }

  private:
    SHFData data;
};
