_Pragma("once")
#ifdef __LINUX__

class RLimit {
  public:
    static unsigned int getCurRtPrio();
    static unsigned int getMaxRtPrio();
    static bool isRtPrioAllowed();
  private:
    RLimit::RLimit() = delete;
};
