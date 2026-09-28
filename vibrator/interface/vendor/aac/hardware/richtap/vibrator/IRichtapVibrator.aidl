package vendor.aac.hardware.richtap.vibrator;

import vendor.aac.hardware.richtap.vibrator.IRichtapCallback;

// Method order and oneway flags match the transaction codes of the stock
// AAC RichTap client (vendorAacHardwareRichtapVibrator.jar); do not reorder.
interface IRichtapVibrator {
    oneway void init(IRichtapCallback callback);
    oneway void setDynamicScale(int scale, IRichtapCallback callback);
    oneway void setF0(int f0, IRichtapCallback callback);
    void stop(IRichtapCallback callback);
    oneway void setAmplitude(int amplitude, IRichtapCallback callback);
    void performHeParam(int interval, int amplitude, int freq, IRichtapCallback callback);
    oneway void off(IRichtapCallback callback);
    oneway void on(int timeoutMs, IRichtapCallback callback);
    int perform(int effectId, byte strength, IRichtapCallback callback);
    oneway void performEnvelope(in int[] envInfo, boolean fastFlag, IRichtapCallback callback);
    oneway void performRtp(in ParcelFileDescriptor hdl, IRichtapCallback callback);
    oneway void performHe(int looper, int interval, int amplitude, int freq, in int[] he, IRichtapCallback callback);
}
