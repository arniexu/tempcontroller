#ifndef __STM32F1xx_H
#define __STM32F1xx_H

#ifndef STM32F10X_MD
#define STM32F10X_MD
#endif

#include "stm32f10x.h"

#ifndef SET_BIT
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#endif
#ifndef READ_BIT
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#endif
#ifndef WRITE_REG
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#endif
#ifndef READ_REG
#define READ_REG(REG)         (REG)
#endif
#ifndef MODIFY_REG
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#endif
#ifndef UNUSED
#define UNUSED(X) ((void)(X))
#endif

#endif /* __STM32F1xx_H */
