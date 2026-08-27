package google.hardware.power.extension.pixel;

@VintfStability
interface IPowerExt {
    oneway void setMode(in @utf8InCpp String mode, in boolean enabled);
    boolean isModeSupported(in @utf8InCpp String mode);
    oneway void setBoost(in @utf8InCpp String boost, in int durationMs);
    boolean isBoostSupported(in @utf8InCpp String boost);
}
