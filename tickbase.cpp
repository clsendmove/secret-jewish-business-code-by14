void AdjustPlayerTimeBase( int simulation_ticks )
{

    if ( simulation_ticks < 0 )
        return;

    auto sv_clockcorrection_msecs = g_cvars.find_var( "sv_clockcorrection_msecs" );
    float flCorrectionSeconds = std::clamp( sv_clockcorrection_msecs->GetFloat( ) / 1000.0f, 0.0f, 1.0f );
    int nCorrectionTicks = TIME_TO_TICKS( flCorrectionSeconds );

    int nIdealFinalTick = g_tf2.m_cl->m_ClockDriftMgr.m_nServerTick + nCorrectionTicks;

    int nEstimatedFinalTick = g_cl.m_local->m_nTickBase( ) + simulation_ticks;

    int too_fast_limit = nIdealFinalTick + nCorrectionTicks;
    int too_slow_limit = nIdealFinalTick - nCorrectionTicks;

    auto process = g_tf2.m_globals->simTicksThisFrame + TIME_TO_TICKS( g_tf2.m_cl->m_NetChannel->GetLatency( FLOW_OUTGOING ) + g_tf2.m_cl->m_NetChannel->GetLatency( FLOW_INCOMING ) );

    if ( nEstimatedFinalTick > too_fast_limit ||
         nEstimatedFinalTick < too_slow_limit )
    {
        int nCorrectedTick = nIdealFinalTick - simulation_ticks + process;

        g_cl.m_local->m_nTickBase( ) = nCorrectedTick;
        g_tf2.m_globals->curtime = TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );
    }
}

void TickBase::correct_tickbase( )
{
    if ( !m_wish_correct )
        return;

    AdjustPlayerTimeBase( g_tf2.m_cl->chokedcommands );

    m_wish_correct = false;
}
