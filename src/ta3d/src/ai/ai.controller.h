/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2005  Roland BROCHARD

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

/*-----------------------------------------------------------------------------\
|                               ai.controller.h                                |
|       This module implements a basic AI with 4 different difficulty levels.  |
|                                                                              |
\-----------------------------------------------------------------------------*/

#ifndef __AI_CONTROLLER_H__
# define __AI_CONTROLLER_H__

# include "../threads/thread.h"
# include "brain.h"
# include "weight.h"

namespace TA3D
{

#define	ORDER_ARMY			0x00			// Order to build an army
#define	ORDER_METAL_P		0x01			// Order to gather metal
#define	ORDER_ENERGY_P		0x02			// Order to gather energy
#define	ORDER_DEFENSE		0x03			// Order to build defensive units ( AA towers, but also atomic weapons ...)
#define	ORDER_FACTORY		0x04			// Order to build factories
#define	ORDER_BUILDER		0x05			// Order to build construction units
#define	ORDER_METAL_S		0x06			// Order to store metal
#define	ORDER_ENERGY_S		0x07			// Order to store energy

#define NB_ORDERS			0x8

#define BRAIN_VALUE_NULL	0x0
#define BRAIN_VALUE_LOW		0x1
#define BRAIN_VALUE_MEDIUM	0x2
#define BRAIN_VALUE_HIGH	0x4
#define BRAIN_VALUE_MAX		0x8

#define BRAIN_VALUE_BITS	0x4			// How many bits are needed to store a value in a neural network ?

#define	AI_TYPE_EASY		0x0
#define AI_TYPE_MEDIUM		0x1
#define AI_TYPE_HARD		0x2
#define AI_TYPE_BLOODY		0x3
#define AI_TYPE_LUA         0x4

    class AI_CONTROLLER :	public ObjectSync,			// Class to manage players controled by AI
                            public Thread
    {
        virtual const char *className() { return "AI_CONTROLLER"; }
    private:
        String			name;			// Attention faudrait pas qu'il se prenne pour quelqu'un!! -> indique aussi le fichier correspondant à l'IA (faut sauvegarder les cervelles)
        BRAIN			decide;			// Neural network to take decision
        BRAIN			anticipate;		// Neural network to make it anticipate
        int			    playerID;		// Identifiant du joueur / all is in the name :)
        uint16			unit_id;		// Unit index to run throught the unit array
        uint16			total_unit;

        byte			AI_type;		// Which AI do we have to use?

        AI_WEIGHT		*weights;		// Vector of weights used to decide what to build
        byte            *enemy_table;   // A table used to speed up some look up
        uint16			nb_units[ NB_AI_UNIT_TYPE ];
        uint16			nb_enemy[ 10 ];				// Hom many units has each enemy ?
        float			order_weight[NB_ORDERS];	// weights of orders
        float			order_attack[ 10 ];			// weights of attack order per enemy player
        std::list<uint16>	builder_list;
        std::list<uint16>	factory_list;
        std::list<uint16>	army_list;
        std::vector< std::list<WEIGHT_COEF> > enemy_list;

    protected:
        void	proc(void*);
        void	signalExitThread();

        bool	thread_running;
        bool	thread_ask_to_stop;

    public:

        AI_CONTROLLER();
        ~AI_CONTROLLER();

    private:
        void init();
        void destroy();
    public:

        void monitor();

        void changeName(const String& newName);		// Change AI's name (-> creates a new file)
        void setPlayerID(int id);
        int getPlayerID();

        void setType(int type);
        int getType();

        void save();
        void loadAI(const String& filename, const int id = 0);

    private:
        void scan_unit();
        void refresh_unit_weights();
        void think();
    };


} // namespace TA3D

#endif // TA3D_XX_AI_H__