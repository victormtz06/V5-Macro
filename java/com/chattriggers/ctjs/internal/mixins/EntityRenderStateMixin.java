package com.chattriggers.ctjs.internal.mixins;


import com.chattriggers.ctjs.internal.IRenderState;
import net.minecraft.client.renderer.entity.state.EntityRenderState;
import net.minecraft.world.entity.Entity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;

@Mixin(EntityRenderState.class)
public abstract class EntityRenderStateMixin implements IRenderState {
    @Unique
    private Entity ctjs$entity;

    @Override
    public void ctjs$setEntity(Entity entity) {
        this.ctjs$entity = entity;
    }

    @Override
    public Entity ctjs$getEntity() {
        return this.ctjs$entity;
    }
}