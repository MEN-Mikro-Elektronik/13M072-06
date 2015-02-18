#************************** MDIS4 device descriptor *************************
#
# m72_max.dsc: Descriptor for M72
# Automatically generated by mdiswiz 0.97a.003-linux-1 from 13m07206.xml
# 2004-08-30
#
#****************************************************************************

M72_1 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M72_PRE

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING A201_1
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 1

    # Define wether PLD is to be loaded at INIT
    # 0 := don't load PLD
    # 1 := load PLD
    PLD_LOAD = U_INT32 1

    # output signal mode, see user manual
    OUT_MODE = U_INT32 0x0

    # output signal setting, see user manual
    # Range: 0x0..0xf
    OUT_SET = U_INT32 0x0
    CHANNEL_0 {

        # counter mode, see user manual
        # Range: 0..10
        CNT_MODE = U_INT32 0

        # counter preload condition, see user manual
        # Range: 0..3
        CNT_PRELOAD = U_INT32 0

        # counter clear condition, see user manual
        # Range: 0..3
        CNT_CLEAR = U_INT32 0

        # counter store condition, see user manual
        # Range: 0..3
        CNT_STORE = U_INT32 0

        # global interrupt facility, see user manual
        # Range: 0..1
        ENB_IRQ = U_INT32 0

        # comparator interrupt condition, see user manual
        # Range: 0..5
        COMP_IRQ = U_INT32 0

        # carry/borrow interrupt condition, see user manual
        # Range: 0..3
        CYBW_IRQ = U_INT32 0

        # Line break interrupt enable, see user manual
        # Range: 0..1
        LBREAK_IRQ = U_INT32 0

        # xIN2 edge interrupt enable, see user manual
        # Range: 0..1
        XIN2_IRQ = U_INT32 0

        # counter preload value
        VAL_PRELOAD = U_INT32 0x0

        # comparator A value
        VAL_COMPA = U_INT32 0x0

        # comparator B value
        VAL_COMPB = U_INT32 0x0

        # mode for read calls
        # 0 := read latched value
        # 1 := wait for ready interrupt
        # 2 := force counter latch
        READ_MODE = U_INT32 2

        # Timeout for read calls in ms
        READ_TIMEOUT = U_INT32 0xffffffff

        # mode for write calls
        # 0 := load preload register
        # 2 := force counter load
        WRITE_MODE = U_INT32 2

        # timer start condition, see user manual
        # Range: 0..1
        TIMER_START = U_INT32 0
    }
    CHANNEL_1 {

        # counter mode, see user manual
        # Range: 0..10
        CNT_MODE = U_INT32 0

        # counter preload condition, see user manual
        # Range: 0..3
        CNT_PRELOAD = U_INT32 0

        # counter clear condition, see user manual
        # Range: 0..3
        CNT_CLEAR = U_INT32 0

        # counter store condition, see user manual
        # Range: 0..3
        CNT_STORE = U_INT32 0

        # global interrupt facility, see user manual
        # Range: 0..1
        ENB_IRQ = U_INT32 0

        # comparator interrupt condition, see user manual
        # Range: 0..5
        COMP_IRQ = U_INT32 0

        # carry/borrow interrupt condition, see user manual
        # Range: 0..3
        CYBW_IRQ = U_INT32 0

        # Line break interrupt enable, see user manual
        # Range: 0..1
        LBREAK_IRQ = U_INT32 0

        # xIN2 edge interrupt enable, see user manual
        # Range: 0..1
        XIN2_IRQ = U_INT32 0

        # counter preload value
        VAL_PRELOAD = U_INT32 0x0

        # comparator A value
        VAL_COMPA = U_INT32 0x0

        # comparator B value
        VAL_COMPB = U_INT32 0x0

        # mode for read calls
        # 0 := read latched value
        # 1 := wait for ready interrupt
        # 2 := force counter latch
        READ_MODE = U_INT32 2

        # Timeout for read calls in ms
        READ_TIMEOUT = U_INT32 0xffffffff

        # mode for write calls
        # 0 := load preload register
        # 2 := force counter load
        WRITE_MODE = U_INT32 2

        # timer start condition, see user manual
        # Range: 0..1
        TIMER_START = U_INT32 0
    }
    CHANNEL_2 {

        # counter mode, see user manual
        # Range: 0..10
        CNT_MODE = U_INT32 0

        # counter preload condition, see user manual
        # Range: 0..3
        CNT_PRELOAD = U_INT32 0

        # counter clear condition, see user manual
        # Range: 0..3
        CNT_CLEAR = U_INT32 0

        # counter store condition, see user manual
        # Range: 0..3
        CNT_STORE = U_INT32 0

        # global interrupt facility, see user manual
        # Range: 0..1
        ENB_IRQ = U_INT32 0

        # comparator interrupt condition, see user manual
        # Range: 0..5
        COMP_IRQ = U_INT32 0

        # carry/borrow interrupt condition, see user manual
        # Range: 0..3
        CYBW_IRQ = U_INT32 0

        # Line break interrupt enable, see user manual
        # Range: 0..1
        LBREAK_IRQ = U_INT32 0

        # xIN2 edge interrupt enable, see user manual
        # Range: 0..1
        XIN2_IRQ = U_INT32 0

        # counter preload value
        VAL_PRELOAD = U_INT32 0x0

        # comparator A value
        VAL_COMPA = U_INT32 0x0

        # comparator B value
        VAL_COMPB = U_INT32 0x0

        # mode for read calls
        # 0 := read latched value
        # 1 := wait for ready interrupt
        # 2 := force counter latch
        READ_MODE = U_INT32 2

        # Timeout for read calls in ms
        READ_TIMEOUT = U_INT32 0xffffffff

        # mode for write calls
        # 0 := load preload register
        # 2 := force counter load
        WRITE_MODE = U_INT32 2

        # timer start condition, see user manual
        # Range: 0..1
        TIMER_START = U_INT32 0
    }
    CHANNEL_3 {

        # counter mode, see user manual
        # Range: 0..10
        CNT_MODE = U_INT32 0

        # counter preload condition, see user manual
        # Range: 0..3
        CNT_PRELOAD = U_INT32 0

        # counter clear condition, see user manual
        # Range: 0..3
        CNT_CLEAR = U_INT32 0

        # counter store condition, see user manual
        # Range: 0..3
        CNT_STORE = U_INT32 0

        # global interrupt facility, see user manual
        # Range: 0..1
        ENB_IRQ = U_INT32 0

        # comparator interrupt condition, see user manual
        # Range: 0..5
        COMP_IRQ = U_INT32 0

        # carry/borrow interrupt condition, see user manual
        # Range: 0..3
        CYBW_IRQ = U_INT32 0

        # Line break interrupt enable, see user manual
        # Range: 0..1
        LBREAK_IRQ = U_INT32 0

        # xIN2 edge interrupt enable, see user manual
        # Range: 0..1
        XIN2_IRQ = U_INT32 0

        # counter preload value
        VAL_PRELOAD = U_INT32 0x0

        # comparator A value
        VAL_COMPA = U_INT32 0x0

        # comparator B value
        VAL_COMPB = U_INT32 0x0

        # mode for read calls
        # 0 := read latched value
        # 1 := wait for ready interrupt
        # 2 := force counter latch
        READ_MODE = U_INT32 2

        # Timeout for read calls in ms
        READ_TIMEOUT = U_INT32 0xffffffff

        # mode for write calls
        # 0 := load preload register
        # 2 := force counter load
        WRITE_MODE = U_INT32 2

        # timer start condition, see user manual
        # Range: 0..1
        TIMER_START = U_INT32 0
    }

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        this keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
# EOF
