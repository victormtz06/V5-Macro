package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.particle.Particle;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(Particle.class)
public interface ParticleAccessor {
    @Accessor(value = "x")
    double getX();

    @Accessor(value = "x")
    void setX(double value);

    @Accessor(value = "y")
    double getY();

    @Accessor(value = "y")
    void setY(double value);

    @Accessor(value = "z")
    double getZ();

    @Accessor(value = "z")
    void setZ(double value);

    @Accessor
    double getXd();

    @Accessor
    void setXd(double value);

    @Accessor
    double getYd();

    @Accessor
    void setYd(double value);

    @Accessor
    double getZd();

    @Accessor
    void setZd(double value);

    @Accessor
    int getAge();

    @Accessor
    void setAge(int value);

    @Accessor
    double getXo();

    @Accessor
    void setXo(double value);

    @Accessor
    double getYo();

    @Accessor
    void setYo(double value);

    @Accessor
    double getZo();

    @Accessor
    void setZo(double value);

    @Accessor
    boolean getRemoved();

    @Accessor
    void setRemoved(boolean value);
}
