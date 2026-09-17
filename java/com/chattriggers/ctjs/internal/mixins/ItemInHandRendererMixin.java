package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.renderer.ItemInHandRenderer;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

@Mixin(ItemInHandRenderer.class)
public class ItemInHandRendererMixin {
    @Redirect(
            method = "tick",
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/LocalPlayer;getMainHandItem()Lnet/minecraft/world/item/ItemStack;"))
    private ItemStack v5$getMainHandItem(LocalPlayer player) {
        LivingEntity target = v5$getSpectatedEntity();
        return target != null ? target.getMainHandItem() : player.getMainHandItem();
    }

    @Redirect(
            method = "tick",
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/LocalPlayer;getOffhandItem()Lnet/minecraft/world/item/ItemStack;"))
    private ItemStack v5$getOffhandItem(LocalPlayer player) {
        LivingEntity target = v5$getSpectatedEntity();
        return target != null ? target.getOffhandItem() : player.getOffhandItem();
    }

    @Redirect(
            method = {"renderHandsWithItems", "submitHandsWithItems"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/LocalPlayer;getAttackAnim(F)F"))
    private float v5$getAttackAnim(LocalPlayer player, float partialTick) {
        LivingEntity target = v5$getSpectatedEntity();
        return target != null ? target.getAttackAnim(partialTick) : player.getAttackAnim(partialTick);
    }

    @Redirect(
            method = {"renderHandsWithItems", "submitHandsWithItems"},
            at = @At(
                    value = "FIELD",
                    target = "Lnet/minecraft/client/player/LocalPlayer;swingingArm:Lnet/minecraft/world/InteractionHand;"))
    private InteractionHand v5$getSwingingArm(LocalPlayer player) {
        LivingEntity target = v5$getSpectatedEntity();
        return target != null ? target.swingingArm : player.swingingArm;
    }

    private LivingEntity v5$getSpectatedEntity() {
        return Client.getSpectatedEntity();
    }
}
