#include "allCtrlRegs_r10b.h"
#include "globals.h"


AllCtrlRegs_r10b::AllCtrlRegs_r10b(int fd) : Test(fd, SPECREV_10b)
{
    // 66 chars allowed:     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    mTestDesc.SetCompliance("revision 1.0b, section 3");
    mTestDesc.SetShort(     "Validate all controller registers syntactically");
    // No string size limit for the long description
    mTestDesc.SetLong(
        "Validates the following; the RO fields which are not implementation "
        "specific contain default values; The RO fields cannot be written; All "
        "ASCII fields only contain chars 0x20 to 0x7e.");
}


AllCtrlRegs_r10b::~AllCtrlRegs_r10b()
{
    ///////////////////////////////////////////////////////////////////////////
    // Allocations taken from the heap and not under the control of the
    // RsrcMngr need to be freed/deleted here.
    ///////////////////////////////////////////////////////////////////////////
}


AllCtrlRegs_r10b::
AllCtrlRegs_r10b(const AllCtrlRegs_r10b &other) : Test(other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
}


AllCtrlRegs_r10b &
AllCtrlRegs_r10b::operator=(const AllCtrlRegs_r10b &other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
    Test::operator=(other);
    return *this;
}


bool
AllCtrlRegs_r10b::RunCoreTest()
{
    /** \verbatim
     * Assumptions:
     * 1) none
     *  \endverbatim
     */
    ValidateDefaultValues();
    ValidateROBitsAfterWriting();
    return true;
}


void
AllCtrlRegs_r10b::ValidateDefaultValues()
{
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    LOG_NRM("Validating default register values");

    // Traverse the ctrl'r registers
    for (int j = 0; j < CTLSPC_FENCE; j++) {
        if (ctlMetrics[j].specRev != mSpecRev)
            continue;
        ValidateCtlRegisterROAttribute((CtlSpc)j);
    }
}


void
AllCtrlRegs_r10b::ValidateROBitsAfterWriting()
{
    uint64_t value;
    uint64_t origValue;
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    LOG_NRM("Validating RO bits after writing");

    /// Traverse the ctrl'r registers
    for (int j = 0; j < CTLSPC_FENCE; j++) {
        if (ctlMetrics[j].specRev != mSpecRev)
            continue;

        // Reserved areas at NOT suppose to be written
        if ((j == CTLSPC_RES1) || (j == CTLSPC_RES2) || (j == CTLSPC_RES3))
            continue;

        LOG_NRM("Validate RO attribute after trying to write 1");
        if (gRegisters->Read((CtlSpc)j, origValue) == false)
            throw exception();
        value = (origValue | ctlMetrics[j].maskRO);
        if (gRegisters->Write((CtlSpc)j, value) == false)
            throw exception();
        ValidateCtlRegisterROAttribute((CtlSpc)j);

        LOG_NRM("Validate RO attribute after trying to write 0");
        value = (origValue & ~ctlMetrics[j].maskRO);
        if (gRegisters->Write((CtlSpc)j, value) == false)
            throw exception();
        ValidateCtlRegisterROAttribute((CtlSpc)j);
    }
}


int
AllCtrlRegs_r10b::ReportOffendingBitPos(uint64_t val, uint64_t expectedVal)
{
    uint64_t bitMask;

    for (int i = 0; i < (int)(sizeof(uint64_t)*8); i++) {
        bitMask = (1 << i);
        if ((val & bitMask) != (expectedVal & bitMask))
            return i;
    }
    return INT_MAX; // there is no mismatch
}


void
AllCtrlRegs_r10b::ValidateCtlRegisterROAttribute(CtlSpc reg)
{
    uint64_t value = 0;
    uint64_t expectedValue = 0;
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    if (ctlMetrics[reg].size > MAX_SUPPORTED_REG_SIZE) {
        for (int k = 0; (k*sizeof(value)) < ctlMetrics[reg].size; k++) {

            if (gRegisters->Read(NVMEIO_BAR01, sizeof(value),
                ctlMetrics[reg].offset + (k * sizeof(value)),
                (uint8_t *)&value) == false) {

                throw exception();
            } else {
                // Ignore the implementation specific bits, and bits that the
                // manufacturer can make a decision as to their type of access
                value &= ~ctlMetrics[reg].impSpec;

                // Verify that the RO bits are set to correct default values, no
                // reset needed to achieve this because there's no way to change.
                value &= ctlMetrics[reg].maskRO;
                expectedValue =
                    (ctlMetrics[reg].dfltValue & ctlMetrics[reg].maskRO);

                if (value != expectedValue) {
                    LOG_ERR("%s RO bit #%d has incorrect value",
                        ctlMetrics[reg].desc,
                        ReportOffendingBitPos(value, expectedValue));
                    throw exception();
                }
            }
        }
    } else if (gRegisters->Read(reg, value) == false) {
        throw exception();
    } else {
        // Ignore the implementation specific bits, and bits that the
        // manufacturer can make a decision as to their type of access RW/RO
        value &= ~ctlMetrics[reg].impSpec;

        // Verify that the RO bits are set to correct default values, no
        // reset needed to achieve this because there's no way to change.
        value &= ctlMetrics[reg].maskRO;
        expectedValue = (ctlMetrics[reg].dfltValue & ctlMetrics[reg].maskRO);

        if (value != expectedValue) {
            LOG_ERR("%s RO bit #%d has incorrect value",
                ctlMetrics[reg].desc,
                ReportOffendingBitPos(value, expectedValue));
            throw exception();
        }
    }
}


