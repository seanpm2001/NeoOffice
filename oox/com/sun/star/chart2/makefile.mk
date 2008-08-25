#*************************************************************************
#
# Copyright 2008 by Planamesa Inc.
#
# NeoOffice - a multi-platform office productivity suite
#
# $RCSfile$
#
# $Revision$
#
# This file is part of NeoOffice.
#
# NeoOffice is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3
# only, as published by the Free Software Foundation.
#
# NeoOffice is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU General Public License
# version 3 along with NeoOffice.  If not, see
# <http://www.gnu.org/licenses/gpl-3.0.txt>
# for a copy of the GPLv3 License.
#
#*************************************************************************

PRJ=..$/..$/..$/..

PRJNAME=oox

TARGET=chart2
PACKAGE=com$/sun$/star$/chart2

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# ------------------------------------------------------------------------

IDLFILES=\
	AxisType.idl \
	AxisOrientation.idl \
	AxisPosition.idl \
	Break.idl \
	CoordinateSystemTypeID.idl \
	CurveStyle.idl \
	DataPointGeometry3D.idl \
	DataPointLabel.idl \
	ExplicitIncrementData.idl \
	ExplicitScaleData.idl \
	ExplicitSubIncrement.idl \
	FillBitmap.idl \
	IncrementData.idl \
	InterpretedData.idl \
	LegendExpansion.idl \
	LegendPosition.idl \
	LegendSymbolStyle.idl \
	LightSource.idl \
	PieChartOffsetMode.idl \
	RelativePosition.idl \
	RelativeSize.idl \
	ScaleData.idl \
	StackingDirection.idl \
	SubIncrement.idl \
	Symbol.idl \
	SymbolStyle.idl \
	TickmarkStyle.idl \
	TransparencyStyle.idl \
	ViewLegendEntry.idl \
	XAxis.idl \
	XCoordinateSystem.idl \
	XCoordinateSystemContainer.idl \
	XChartDocument.idl \
	XChartShape.idl \
	XChartShapeContainer.idl \
	XChartType.idl \
	XChartTypeContainer.idl \
	XChartTypeManager.idl \
	XChartTypeTemplate.idl \
	XColorScheme.idl \
	XDataInterpreter.idl \
	XDataSeries.idl \
	XDataSeriesContainer.idl \
	XDiagram.idl \
	XDiagramProvider.idl \
	XFormattedString.idl \
	XInternalDataProvider.idl \
	XLabeled.idl \
	XLegend.idl \
	XLegendEntry.idl \
	XLegendSymbolProvider.idl \
	XPlotter.idl \
	XRegressionCurve.idl \
	XRegressionCurveCalculator.idl \
	XRegressionCurveContainer.idl \
	XScaling.idl \
	XTarget.idl \
	XTitle.idl \
	XTitled.idl \
	XTransformation.idl \
	XUndoManager.idl \
	XUndoSupplier.idl \
	XUndoHelper.idl

# ------------------------------------------------------------------

.INCLUDE :  target.mk
.INCLUDE :  $(PRJ)$/util$/target.pmk
