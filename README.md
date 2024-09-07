# MDFastBinding
A versatile and performant alternative to UMG property bindings for designer-friendly workflows.
The goal was to build a tool that allows mutating raw data into a form that can drive visuals, all within the editor, while staying performant.

## Requirements
MDFastBinding now only supports Unreal 5.1 and later. See the [UE-4.27-5.0 tag](https://github.com/DoubleDeez/MDFastBinding/tree/UE-4.27-5.0) for older versions.

## Setup
1. Clone this repo into your Plugins folder or download a [pre-compiled release](https://github.com/DoubleDeez/MDFastBinding/releases).
2. Launch the editor, enable the plugin, and restart the editor if necessary.
3. Create a blueprint that extends your class (or `UserWidget`)
4. In the editor of your blueprint, you should see a "Binding Editor" button in the toolbar:

![Unreal 5 Binding Icon](Resources/readme-binding-editor-ue5.png)

5. This will open a new tab with an empty graph and an empty list on the left side. Create a new binding by click the "Add Binding" button in the bottom left and name your new binding.

![Unreal 5 Binding Icon](Resources/readme-add-binding-ue5.png)

6. Now on the right side, you can right-click and start adding nodes to your new binding.
Here's a screenshot binding 2 health variables to a progress bar widget's percent:

![Example of binding a health bar percentage](Resources/readme-binding-example.png)

### Binding Widget Properties

You can use the normal property binding menu in the Widget Designer to create Fast Bindings. This will auto-populate a binding that sets the property you selected and opens the binding editor drawer. The widget you have selected must have `Is Variable` checked for the binding menu to display.

When a property has an existing binding, the binding menu will show the binding's status (Error or Performance rating) and the binding's name.

![Example of creating and deleting fast bindings in details view](Resources/readme-umg-property-bindings.gif)

## Performance
Check out the [Performance](https://github.com/DoubleDeez/MDFastBinding/wiki/Performance) wiki page for details on how to make performant bindings.

## Debugging
The binding editor integrates with the blueprint debugger to support displaying values of the binding node pins in real time and highlight values that are changing in the binding graph, based on the selected debug object in the blueprint editor. See the [Debugging](https://github.com/DoubleDeez/MDFastBinding/wiki/Debugging) wiki page for more details.

![GIF of the binding graph animating wires that change and watched values](Resources/readme-binding-debug.gif)

## Previewing in the Widget Designer
Bindings on User Widgets can be previewed in the widget designer by clicking the **Enable Design-Time Bindings** button in the toolbar. This is useful for previewing bindings that initialize visuals or bindings that interact with widget animations.

In this example, the square's default color is white, but it's bound to a color variable with a default of light-red:

![GIF previewing design-time bindings](Resources/readme-design-time-binding.gif)

With design-time bindings enabled, functionality can be previewed by using [CallInEditor Functions](https://benui.ca/unreal/ufunction/#callineditor):

![GIF previewing a CallInEditor function working with design-time bindings](Resources/readme-callineditor-binding.gif)
