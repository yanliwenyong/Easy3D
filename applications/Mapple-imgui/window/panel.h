/********************************************************************
 * Copyright (C) 2015 Liangliang Nan <liangliang.nan@gmail.com>
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++ library
 *      for processing and rendering 3D data.
 *      Journal of Open Source Software, 6(64), 3255, 2021.
 * ------------------------------------------------------------------
 *
 * Easy3D is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 *
 * Easy3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#ifndef EASY3D_VIEWER_H
#define EASY3D_VIEWER_H

#include <string>
#include <vector>


namespace easy3d {

	class  ViewerImGui;
	class  Plugin;

	// A control panel provides means for interacting with 3D viewers,
	// e.g., open/load file, change display parameters.

	// All control panels MUST be derived from this class and may implement 
	// any/all the callbacks marked `virtual` here.
	// NOTE: Return true of callbacks tells the viewer that the event has been 
	// handled and that it should not be passed to the viewer and other plugins.

	class Panel
	{
	public:
		Panel(ViewerImGui* viewer, const std::string& title);

        bool add_plugin(Plugin* plugin);

    protected:
		// This function is called before the viewer is destroyed
		// (i.e., a valid rendering context still exists)
		virtual void cleanup();

		// Draw the widgets of this panel.
		virtual void draw_widgets();

		// Mouse IO

		// This function is called when the mouse button is pressed
		// - button can be GLFW_MOUSE_BUTTON_{LEFT, MIDDLE, or RIGHT}
		// - modifiers is a bitfield that might one or more of the following bits: 
		//   GLFW_MOD_{SHIFT, CONTROL, and ALT}	
		virtual bool mouse_press(int button, int modifier);
		// This function is called when the mouse button is released
		// - button can be GLFW_MOUSE_BUTTON_{LEFT, MIDDLE, or RIGHT}
		// - modifiers is a bitfield that might one or more of the following bits: 
		//   GLFW_MOD_{SHIFT, CONTROL, and ALT}	
		virtual bool mouse_release(int button, int modifier);
		// This function is called every time the mouse is moved
		// - mouse_x and mouse_y are the new coordinates of the mouse pointer in 
		//   screen coordinates
		virtual bool mouse_move(int mouse_x, int mouse_y);
		// This function is called every time the scroll wheel is moved
		virtual bool mouse_scroll(double delta_y);

		// Keyboard IO

		// This function is called when a keyboard key is pressed. This function 
		// reveals the actual character being sent (not just the physical key).
		// - codepoint is in native endian UTF-32 format
		virtual bool char_input(unsigned int key);
		// This function is called when a keyboard key is pressed. Unlike char_input,
		// this will not reveal the actual character being sent (just the physical key).
		// - modifiers is a bitfield that might one or more of the following bits: 
		//   GLFW_MOD_{SHIFT, CONTROL, and ALT}	
		virtual bool key_press(int key, int modifiers);
		// This function is called when a keyboard key is released. Unlike char_input,
		// this will not reveal the actual character being sent (just the physical key).
		// - modifiers is a bitfield that might one or more of the following bits: 
		//   GLFW_MOD_{SHIFT, CONTROL, and ALT}	
		virtual bool key_release(int key, int modifiers);

	protected:

		// Draw the control panel. Its contents are drawn in draw_widgets().
		// This function is called in the draw procedure
		bool  draw();

	protected:
        ViewerImGui*	viewer_;
		std::string name_;

		// List of registered plugins
		std::vector<Plugin*> plugins_;

		bool  visible_;	// a panel can be hidden/shown

        friend class ViewerImGui;
	};

}

#endif // EASY3D_VIEWER_H
