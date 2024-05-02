//
//  EgtbDbWritting.h
//  EgtbGen
//
//  Created by Tony Pham on 20/9/18.
//

#ifndef EgtbDbWritting_h
#define EgtbDbWritting_h

#include "Obj.h"
#include "../egtb/EgtbDb.h"
#include "../egtb/Egtb.h"

#include "ExtEgtbDb.h"
#include "Extensions.h"
#include "EgtbFileWritting.h"
#include "Lib.h"

namespace egtb {

    class EgtbDbWritting : public ExtEgtbDb, public Obj {
    public:
        virtual EgtbFile* createEgtbFile() const;

        bool setupPieceList(EgtbFile* egtbFile, EgtbBoard& board, i64 idx, bslib::FlipMode flip, bslib::Side strongsider);

        bool setup(EgtbFile* egtbFile, EgtbBoard& board, i64 idx, bslib::FlipMode flip = bslib::FlipMode::none, bslib::Side strongsider = bslib::Side::white) const;
        
        i64 getIdx(const EgtbBoard& board);

        std::string toString() const {
            std::ostringstream stringStream;
            auto cnt = 0;
            i64 sz = 0;
            for(auto && egtb : getEgtbFileVec()) {
                cnt++;
                sz += egtb->getSize();
                stringStream << cnt << ") " << egtb->getName() << ", " << Lib::formatString(egtb->getSize()) << std::endl;
            }

            stringStream  << "Total: #" << cnt << ", sz: " << Lib::formatString(sz) << std::endl;
            return stringStream.str();
        }
        
    };

} // namespace egtb

#endif /* EgtbDbWritting_h */
