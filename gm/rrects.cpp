/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#if SK_SUPPORT_GPU
#include "GrTest.h"
#include "effects/GrRRectEffect.h"
#endif
#include "SkDevice.h"
#include "SkRRect.h"

namespace skiagm {

///////////////////////////////////////////////////////////////////////////////

class RRectGM : public GM {
public:
    enum Type {
        kBW_Draw_Type,
        kAA_Draw_Type,
        kBW_Clip_Type,
        kAA_Clip_Type,
        kEffect_Type,
    };
    RRectGM(Type type) : fType(type) {
        this->setBGColor(0xFFDDDDDD);
        this->setUpRRects();
    }

protected:
    SkString onShortName() SK_OVERRIDE {
        SkString name("rrect");
        switch (fType) {
            case kBW_Draw_Type:
                name.append("_draw_bw");
                break;
            case kAA_Draw_Type:
                name.append("_draw_aa");
                break;
            case kBW_Clip_Type:
                name.append("_clip_bw");
                break;
            case kAA_Clip_Type:
                name.append("_clip_aa");
                break;
            case kEffect_Type:
                name.append("_effect");
                break;
        }
        return name;
    }

    virtual SkISize onISize() SK_OVERRIDE { return make_isize(kImageWidth, kImageHeight); }

    virtual uint32_t onGetFlags() const SK_OVERRIDE {
        if (kEffect_Type == fType) {
            return kGPUOnly_Flag;
        } else {
            return 0;
        }
    }

    virtual void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        int numRRects = kNumRRects;
#if SK_SUPPORT_GPU
        SkBaseDevice* device = canvas->getTopDevice();
        GrContext* context = NULL;
        GrRenderTarget* rt = device->accessRenderTarget();
        if (NULL != rt) {
            context = rt->getContext();
        }
        if (kEffect_Type == fType && NULL == context) {
            return;
        }
        if (kEffect_Type == fType) {
            numRRects *= kGrEffectEdgeTypeCnt;
        }
#endif

        SkPaint paint;
        if (kAA_Draw_Type == fType) {
            paint.setAntiAlias(true);
        }

        static const SkRect kMaxTileBound = SkRect::MakeWH(SkIntToScalar(kTileX), SkIntToScalar(kTileY));

        int curRRect = 0;
        for (int y = 1; y < kImageHeight; y += kTileY) {
            for (int x = 1; x < kImageWidth; x += kTileX) {
                if (curRRect >= numRRects) {
                    break;
                }
                int rrectIdx = curRRect % kNumRRects;
                SkASSERT(kMaxTileBound.contains(fRRects[rrectIdx].getBounds()));

                canvas->save();
                    canvas->translate(SkIntToScalar(x), SkIntToScalar(y));
                    if (kEffect_Type == fType) {
#if SK_SUPPORT_GPU
                        GrTestTarget tt;
                        context->getTestTarget(&tt);
                        if (NULL == tt.target()) {
                            SkDEBUGFAIL("Couldn't get Gr test target.");
                            return;
                        }
                        GrDrawState* drawState = tt.target()->drawState();

                        SkRRect rrect = fRRects[rrectIdx];
                        rrect.offset(SkIntToScalar(x), SkIntToScalar(y));
                        GrEffectEdgeType edgeType = (GrEffectEdgeType) (curRRect / kNumRRects);
                        SkAutoTUnref<GrEffectRef> effect(GrRRectEffect::Create(edgeType, rrect));
                        if (effect) {
                            drawState->addCoverageEffect(effect);
                            drawState->setIdentityViewMatrix();
                            drawState->setRenderTarget(rt);
                            drawState->setColor(0xff000000);

                            SkRect bounds = rrect.getBounds();
                            bounds.outset(2.f, 2.f);

                            tt.target()->drawSimpleRect(bounds);
                        }
#endif
                    } else if (kBW_Clip_Type == fType || kAA_Clip_Type == fType) {
                        bool aaClip = (kAA_Clip_Type == fType);
                        canvas->clipRRect(fRRects[rrectIdx], SkRegion::kReplace_Op, aaClip);
                        canvas->drawRect(kMaxTileBound, paint);
                    } else {
                        canvas->drawRRect(fRRects[rrectIdx], paint);
                    }
                    ++curRRect;
                canvas->restore();
            }
        }
    }

    void setUpRRects() {
        // each RRect must fit in a 0x0 -> (kTileX-2)x(kTileY-2) block. These will be tiled across
        // the screen in kTileX x kTileY tiles. The extra empty pixels on each side are for AA.

        // simple cases
        fRRects[0].setRect(SkRect::MakeWH(kTileX-2, kTileY-2));
        fRRects[1].setOval(SkRect::MakeWH(kTileX-2, kTileY-2));
        fRRects[2].setRectXY(SkRect::MakeWH(kTileX-2, kTileY-2), 10, 10);
        fRRects[3].setRectXY(SkRect::MakeWH(kTileX-2, kTileY-2), 10, 5);
        // small circular corners are an interesting test case for gpu clipping
        fRRects[4].setRectXY(SkRect::MakeWH(kTileX-2, kTileY-2), 1, 1);
        fRRects[5].setRectXY(SkRect::MakeWH(kTileX-2, kTileY-2), 0.5f, 0.5f);
        fRRects[6].setRectXY(SkRect::MakeWH(kTileX-2, kTileY-2), 0.2f, 0.2f);

        // The first complex case needs special handling since it is a square
        fRRects[kNumSimpleCases].setRectRadii(SkRect::MakeWH(kTileY-2, kTileY-2), gRadii[0]);
        for (size_t i = 1; i < SK_ARRAY_COUNT(gRadii); ++i) {
            fRRects[kNumSimpleCases+i].setRectRadii(SkRect::MakeWH(kTileX-2, kTileY-2), gRadii[i]);
        }
    }

private:
    Type fType;

    static const int kImageWidth = 640;
    static const int kImageHeight = 480;

    static const int kTileX = 80;
    static const int kTileY = 40;

    static const int kNumSimpleCases = 7;
    static const int kNumComplexCases = 23;
    static const SkVector gRadii[kNumComplexCases][4];

    static const int kNumRRects = kNumSimpleCases + kNumComplexCases;
    SkRRect fRRects[kNumRRects];

    typedef GM INHERITED;
};

// Radii for the various test cases. Order is UL, UR, LR, LL
const SkVector RRectGM::gRadii[kNumComplexCases][4] = {
    // a circle
    { { kTileY, kTileY }, { kTileY, kTileY }, { kTileY, kTileY }, { kTileY, kTileY } },

    // odd ball cases
    { { 8, 8 }, { 32, 32 }, { 8, 8 }, { 32, 32 } },
    { { 16, 8 }, { 8, 16 }, { 16, 8 }, { 8, 16 } },
    { { 0, 0 }, { 16, 16 }, { 8, 8 }, { 32, 32 } },

    // UL
    { { 30, 30 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    { { 30, 15 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    { { 15, 30 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },

    // UR
    { { 0, 0 }, { 30, 30 }, { 0, 0 }, { 0, 0 } },
    { { 0, 0 }, { 30, 15 }, { 0, 0 }, { 0, 0 } },
    { { 0, 0 }, { 15, 30 }, { 0, 0 }, { 0, 0 } },

    // LR
    { { 0, 0 }, { 0, 0 }, { 30, 30 }, { 0, 0 } },
    { { 0, 0 }, { 0, 0 }, { 30, 15 }, { 0, 0 } },
    { { 0, 0 }, { 0, 0 }, { 15, 30 }, { 0, 0 } },

    // LL
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 30, 30 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 30, 15 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 15, 30 } },

    // over-sized radii
    { { 0, 0 }, { 100, 400 }, { 0, 0 }, { 0, 0 } },
    { { 0, 0 }, { 400, 400 }, { 0, 0 }, { 0, 0 } },
    { { 400, 400 }, { 400, 400 }, { 400, 400 }, { 400, 400 } },

    // circular corner tabs
    { { 0, 0 }, { 20, 20 }, { 20, 20 }, { 0, 0 } },
    { { 20, 20 }, { 20, 20 }, { 0, 0 }, { 0, 0 } },
    { { 0, 0 }, { 0, 0 }, { 20, 20 }, { 20, 20 } },
    { { 20, 20 }, { 0, 0 }, { 0, 0 }, { 20, 20 } },
};

///////////////////////////////////////////////////////////////////////////////

DEF_GM( return new RRectGM(RRectGM::kAA_Draw_Type); )
DEF_GM( return new RRectGM(RRectGM::kBW_Draw_Type); )
DEF_GM( return new RRectGM(RRectGM::kAA_Clip_Type); )
DEF_GM( return new RRectGM(RRectGM::kBW_Clip_Type); )
#if SK_SUPPORT_GPU
DEF_GM( return new RRectGM(RRectGM::kEffect_Type); )
#endif

}
