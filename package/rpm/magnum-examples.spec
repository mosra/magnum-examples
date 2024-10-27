Name: magnum-examples
Version: 2020.06.262.g11b0c81d
Release: 1
Summary: Examples for the Magnum C++11 graphics engine
License: MIT
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires: magnum, magnum-plugins, magnum-integration, magnum-extras, Box2D
BuildRequires: magnum-devel, magnum-integration-devel, magnum-extras-devel, cmake, git, gcc-c++, Box2D-devel
Source1: https://github.com/ocornut/imgui/archive/v1.88.zip

%description
Here are various examples for the Magnum C++11 graphics engine, demonstrating
its features, usage and capabilities.

%prep
%setup -c -n %{name}-%{version}

%build
unzip %{SOURCE1} -d %{_builddir}

mkdir build && cd build
# Configure CMake
cmake ../%{name}-%{version} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DIMGUI_DIR=%{_builddir}/imgui-1.88 \
  -DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=ON \
  -DMAGNUM_WITH_ARCBALL_EXAMPLE=ON \
  -DMAGNUM_WITH_AREALIGHTS_EXAMPLE=ON \
  -DMAGNUM_WITH_AUDIO_EXAMPLE=ON \
  -DMAGNUM_WITH_BOX2D_EXAMPLE=ON \
  -DMAGNUM_WITH_BULLET_EXAMPLE=ON \
  -DMAGNUM_WITH_CUBEMAP_EXAMPLE=ON \
  -DMAGNUM_WITH_DART_EXAMPLE=OFF \
  -DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=ON \
  -DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=ON \
  -DMAGNUM_WITH_IMGUI_EXAMPLE=ON \
  -DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=ON \
  -DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=ON \
  -DMAGNUM_WITH_OCTREE_EXAMPLE=ON \
  -DMAGNUM_WITH_PICKING_EXAMPLE=ON \
  -DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON \
  -DMAGNUM_WITH_RAYTRACING_EXAMPLE=ON \
  -DMAGNUM_WITH_SHADOWS_EXAMPLE=ON \
  -DMAGNUM_WITH_TEXT_EXAMPLE=ON \
  -DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=ON \
  -DMAGNUM_WITH_TEXTUREDTRIANGLE_VULKAN_EXAMPLE=ON \
  -DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON \
  -DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=ON \
  -DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF \
  -DMAGNUM_WITH_TRIANGLE_VULKAN_EXAMPLE=ON \
  -DMAGNUM_WITH_VIEWER_EXAMPLE=ON

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
cd build
make DESTDIR=$RPM_BUILD_ROOT install
strip $RPM_BUILD_ROOT/%{_bindir}/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}/imgui-1.88

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/magnum

%doc %{name}-%{version}/COPYING

%changelog
# TODO: changelog
