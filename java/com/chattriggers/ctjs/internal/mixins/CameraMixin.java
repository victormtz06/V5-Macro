package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.Camera;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.util.Mth;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Camera.class)
public abstract class CameraMixin {
    @Shadow
    private boolean detached;

    @Shadow
    protected abstract void setPosition(Vec3 pos);

    @Shadow
    protected abstract void setRotation(float yaw, float pitch);

    @Inject(method = "alignWithEntity", at = @At("TAIL"))
    private void v5$applyCameraOverride(float partialTick, CallbackInfo ci) {
        Float yaw = Client.getCameraYaw();
        Float pitch = Client.getCameraPitch();
        if (Client.isFreelook() && yaw != null && pitch != null) {
            LocalPlayer player = Minecraft.getInstance().player;
            if (player != null) {
                float yawRadians = yaw * Mth.DEG_TO_RAD;
                float pitchRadians = pitch * Mth.DEG_TO_RAD;
                float cosPitch = Mth.cos(pitchRadians);
                double distance = Client.getFreelookDistance();
                Vec3 eyePos = player.getEyePosition(partialTick);
                this.setPosition(eyePos.add(
                        Mth.sin(yawRadians) * cosPitch * distance,
                        Mth.sin(pitchRadians) * distance,
                        -Mth.cos(yawRadians) * cosPitch * distance));
            }
        } else {
            Vec3 position = Client.getCameraPosition();
            if (position != null) this.setPosition(position);
        }

        if (yaw != null && pitch != null) this.setRotation(yaw, pitch);

        if (Client.isFreecam()) this.detached = true;
    }
}
