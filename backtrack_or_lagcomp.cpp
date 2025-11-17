#include "lagcomp.h"
#include "anim.h"
#include "cvars.h"

LagComp g_lagcomp;

void LagComp::update( )
{
    auto sv_unlag = g_cvars.find_var( "sv_unlag" );
    auto sv_lagcompensation_teleport_dist = g_cvars.find_var( "sv_lagcompensation_teleport_dist" );
    auto sv_maxunlag = g_cvars.find_var( "sv_maxunlag" );

    if ( ( g_tf2.m_globals->maxClients <= 1 ) || !sv_unlag->GetBool( ) )
    {
        records.clear( );
        return;
    }

    auto nci = g_tf2.m_cl->m_NetChannel;

    if ( !nci )
    {
        records.clear( );
        return;
    }

    m_flTeleportDistanceSqr = sv_lagcompensation_teleport_dist->GetFloat( ) * sv_lagcompensation_teleport_dist->GetFloat( );

    float curtime = g_tf2.m_globals->curtime + nci->GetLatency( FLOW_INCOMING ) + nci->GetLatency( FLOW_OUTGOING );
    float flDeadtime = curtime - sv_maxunlag->GetFloat( );

    for ( auto i = 0; i <= g_tf2.m_ent_list->GetHighestEntityIndex( ); ++i )
    {
        auto client = g_tf2.m_ent_list->GetClientEntity( i );
        if ( !client )
        {
            continue;
        }

        if ( client->IsDormant( ) )
        {
            continue;
        }

        auto entity = client->GetBaseEntity( );
        if ( !entity )
        {
            continue;
        }

        auto client_class = entity->GetClientClass( );
        if ( !client_class )
        {
            continue;
        }

        switch ( client_class->GetClassID( ) )
        {
            case ETFClassID::CTFPlayer:
            {
                auto player = entity->As< C_TFPlayer >( );
                if ( player->entindex( ) == g_cl.m_local->entindex( ) )
                    break;

                if ( player->m_iTeamNum( ) == g_cl.m_local->m_iTeamNum( ) )
                    break;

                auto &track = records[ i ];

                if ( player->deadflag( ) || player->m_iHealth( ) < 0 )
                {
                    if ( !track.empty( ) )
                        track.clear( );

                    continue;
                }

                while ( !track.empty( ) )
                {
                    if ( track.back( ).m_flSimulationTime >= flDeadtime )
                    {
                        break;
                    }

                    track.pop_back( );
                }

                if ( !track.empty( ) )
                {
                    if ( track.front( ).m_flSimulationTime >= player->m_flSimulationTime( ) )
                        continue;
                }

                LagRecord record;
                record.m_flSimulationTime = player->m_flSimulationTime( );
                record.m_vecOrigin = player->m_vecOrigin( );
                record.m_vecAngles = player->m_angEyeAngles( );

                track.insert( track.begin( ), record );

                break;
            }
            default: break; // lol who else do we need to backtrack? sentries? )
        }
    }
}

float LagComp::get_lerp_time( )
{
    static ConVar *sv_minupdaterate = g_cvars.find_var( "sv_minupdaterate" );
    static ConVar *sv_maxupdaterate = g_cvars.find_var( "sv_maxupdaterate" );
    static ConVar *cl_updaterate = g_cvars.find_var( "cl_updaterate" );
    static ConVar *cl_interpolate = g_cvars.find_var( "cl_interpolate" );
    static ConVar *cl_interp_ratio = g_cvars.find_var( "cl_interp_ratio" );
    static ConVar *cl_interp = g_cvars.find_var( "cl_interp" );
    static ConVar *sv_client_min_interp_ratio = g_cvars.find_var( "sv_client_min_interp_ratio" );
    static ConVar *sv_client_max_interp_ratio = g_cvars.find_var( "sv_client_max_interp_ratio" );

    float lerp_time = 0.f;

    if ( cl_interpolate->GetInt( ) != 0 )
    {
        int update_rate = std::clamp( cl_updaterate->GetInt( ), ( int )sv_minupdaterate->GetFloat( ), ( int )sv_maxupdaterate->GetFloat( ) );

        float lerp_ratio = cl_interp_ratio->GetFloat( );
        if ( lerp_ratio == 0 )
            lerp_ratio = 1.f;

        float lerp_amount = cl_interp->GetFloat( );

        if ( sv_client_min_interp_ratio && sv_client_max_interp_ratio && sv_client_min_interp_ratio->GetFloat( ) != -1.f )
        {
            lerp_ratio = std::clamp( lerp_ratio, sv_client_min_interp_ratio->GetFloat( ), sv_client_max_interp_ratio->GetFloat( ) );
        } else
        {
            if ( lerp_ratio == 0.f )
                lerp_ratio = 1.f;
        }

        lerp_time = std::max( lerp_amount, lerp_ratio / update_rate );
    }

    return lerp_time;
}

bool LagComp::is_delta_too_big( const Vector &from, const Vector &to, int i )
{
    auto d = ( from - to );
    m_dist[ i ] = glm::length( d );
    auto dist = d.x * d.x + d.y * d.y;
    return dist > m_flTeleportDistanceSqr;
}

void LagComp::get_latest_record( C_TFPlayer *entity, LagRecord &out_record )
{
    if ( !entity )
        return;

    auto index = entity->entindex( );

    auto &track = records[ index ];
    if ( track.empty( ) )
        return;

    Vector prev_origin = entity->GetAbsOrigin( );

    if ( !g_cl.m_cmd )
    {
        return;
    }

    float correct = 0.f;
    auto nci = g_tf2.m_cl->m_NetChannel;

    if ( !nci )
    {
        return;
    }

    correct += nci->GetLatency( FLOW_INCOMING );
    correct += nci->GetLatency( FLOW_OUTGOING );
    correct += get_lerp_time( );
    correct = std::clamp( correct, 0.f, 1.f );

    float curtime = g_tf2.m_globals->curtime + correct;

    for ( const auto &record : track )
    {
        if ( is_delta_too_big( record.m_vecOrigin, prev_origin, entity->entindex( ) ) )
        {
            m_broke_lagcomp = true;
            break;
        }

        auto delta = abs( correct - ( curtime - record.m_flSimulationTime ) );

        if ( delta >= 0.2 + correct )
            break;

        m_broke_lagcomp = false;
        prev_origin = record.m_vecOrigin;
        out_record = record;
    }
}
