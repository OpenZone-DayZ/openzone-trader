// The stand-in wallet's receive gate. Written the way the reference server's
// wallet is: it names the classes it takes and never asks its parent class, so
// it does not check whether the wallet itself lies inside a bag. Whether the
// ENGINE then lets a note into such a wallet is what the probe measures.
//
// OZT_ProbeBadge has no script class of its own and inherits this one through
// its config parent.
class OZT_ProbeWallet extends Inventory_Base
{
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (item.IsKindOf("OZT_ProbeNote"))
            return true;
        if (item.IsKindOf("Nail"))
            return true;
        return false;
    }
}
