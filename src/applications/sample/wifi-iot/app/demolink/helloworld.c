/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos_init.h"
#include "demosdk.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define N 100

void DemoSdkMain(void)
{
    char* chs = "DemoSdkMain";
    DemoSdkEntry();
    int size = 2048;
    char* str = (char*)malloc(sizeof(char)*size);
    Time_DelayMs(100);
    memset(str, 0, size);
    memcpy(str, chs, strlen(chs));

    for (int i = 0; i < N; i++) {
        memset(str, 0, 1);
    }
    

    free(str);
}

SYS_RUN(DemoSdkMain);
