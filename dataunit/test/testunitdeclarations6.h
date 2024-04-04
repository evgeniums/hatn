#pragma once

HDU_V2_UNIT(subunit_names_and_descr,
            HDU_V2_FIELD(dataunit_field,names_and_descr::TYPE,1)
            HDU_V2_FIELD(dataunit_field_descr,names_and_descr::TYPE,2)
            HDU_V2_FIELD(external_field,names_and_descr::TYPE,3)
            HDU_V2_FIELD(external_field_descr,names_and_descr::TYPE,4)
            HDU_V2_FIELD(embedded_field,names_and_descr::TYPE,5)
            HDU_V2_FIELD(embedded_field_descr,names_and_descr::TYPE,6)
            )

HDU_V2_UNIT(embedded_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )
HDU_V2_UNIT(shared_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )

HDU_V2_UNIT(with_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )
