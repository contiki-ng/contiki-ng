
#ifndef SMARTRF_FXN_H
#define SMARTRF_FXN_H

typedef struct {
    int rfMode;
    void (*cpePatchFxn)(void);
    void (*mcePatchFxn)(void);
    void (*rfePatchFxn)(void);

} RF_Mode;

#endif

