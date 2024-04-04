#pragma once

HDU_V2_UNIT(with_unit2,
            HDU_V2_FIELD(f0,simple_int8::TYPE,1)
            )
HDU_V2_UNIT(with_unit3,
            HDU_V2_FIELD(f0,with_unit2::TYPE,1)
            )
HDU_V2_UNIT(with_unit4,
            HDU_V2_FIELD(f0,with_unit3::TYPE,1)
            HDU_V2_FIELD(f1,with_unit2::TYPE,2)
            )
