JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("Bone").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isBone");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("type");const auto nativeStatementTemp17=JSValue("Bone");setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
return nativeStatementTemp4;
}
